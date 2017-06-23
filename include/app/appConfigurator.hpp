/** \file appConfigurator.hpp
 * \author Jared R. Males
 * \brief An application configuration manager
 *
 */

#ifndef __appConfigurator_hpp__
#define __appConfigurator_hpp__

#include "../stringUtils.hpp"

#include "clOptions.hpp"
#include "iniFile.hpp"


#include <list>

namespace mx
{
   
/// A configuration target
/** Specifies the details of a configuration target, which is a value that can be set from the command line and/or a config file.
  * A target has a name used as a key for accessing it, and defines how it is set with short and long command line options and the section and
  * key for the config file.  Can also include help message details.
  * 
  */ 
struct configTarget
{
   std::string name; ///<The name of the target
   std::string shortOpt; ///< The command-line short option (e.g. "f" for -f)
   std::string longOpt; ///< The command-line long option (e.g. "file" for --file)
   int clType; ///< The command-line option type, argType::false, argType::true, argType::optional, argType::required
   std::string section; ///< The config file section name, can be empty ""
   std::string keyword; ///< The config file keyword, read in a "keyword=value" pair
   bool set; ///< true if the value has been set by the configuration, use to distinguish empty strings
   std::vector<std::string> values; ///< holds the values in the order they are set by the configuration

   bool isRequired; ///< Whether or not this is option is required to be set.
   std::string helpType;         ///< The type to display in the help message.
   std::string helpExplanation;  ///< The explanation to display in the help message.
   
   /// Default c'tor
   configTarget()
   {
   }

   /// Construct and set values
   configTarget( const std::string &n,  ///< [in] The name of the target
                 const std::string &so, ///< [in] The command-line short option (e.g. "f" for -f)
                 const std::string &lo, ///< [in] The command-line long option (e.g. "file" for --file)
                 int clt,               ///< [in] The command-line option type, argType::false, argType::true, argType::optional, argType::required
                 const std::string & s, ///< [in] The config file section name, can be empty ""
                 const std::string & kw, ///< [in] The config file keyword, read in a "keyword=value" pair
                 bool isReq = false, ///< [in] Whether or not this is option is required to be set
                 const std::string & ht = "",  ///< [in] The type to display in the help message
                 const std::string & he = ""  ///< [in] The explanation to display in the help message
               )
   {
      name = n;
      shortOpt = so;
      longOpt = lo;
      clType = clt;
      section = s;
      keyword = kw;
      
      isRequired = isReq;
      helpType = ht;
      helpExplanation = he;
      
      set = false;
   }
};

/// Class to manage a set of configurable values, and read their values from config/ini files and the command line.
/** \bug a config=value pair listed in a conf file twice seems to cause a failure, even if they are the same value.
 */
struct appConfigurator
{
   typedef std::unordered_map<std::string, configTarget>::iterator targetIterator;
   typedef std::list<configTarget>::iterator clOnlyTargetIterator;
   
   std::unordered_map<std::string, configTarget> targets;
   std::list<configTarget> clOnlyTargets;
   
   
   std::vector<std::string> nonOptions;
      
   /// Add a configTarget
   /** Note that if name is a duplicate but the section and keyword are empty, it is handled as command-line only.
     */
   void add(configTarget tgt)
   {
      //First check for duplicate name and command line only
      if(targets.count(tgt.name) > 0 && tgt.section == "" && tgt.keyword == "")
      {
         clOnlyTargets.push_back(tgt);
      }
      else
      {
         targets.insert({tgt.name, tgt});
      }
      
      //targets.insert({tgt.name, tgt});
      //clOnlyTargets.push_back(tgt);
   }
   
   ///Parse the command line, updating the targets
   void parseCommandLine(int argc, char ** argv)
   {
      if(argc == 0) return;
      
      clOptions clOpts;

      targetIterator it;

      for(it = targets.begin(); it != targets.end(); ++it)
      {
         if(it->second.shortOpt == "" && it->second.longOpt == "") continue;
         
         clOpts.add(it->second.name,it->second.shortOpt.c_str(),it->second.longOpt.c_str(), it->second.clType);
      }
      
      clOnlyTargetIterator cloit;
      for(cloit = clOnlyTargets.begin(); cloit != clOnlyTargets.end(); ++cloit)
      {
         if(cloit->shortOpt == "" && cloit->longOpt == "") continue;
         
         clOpts.add(cloit->name,cloit->shortOpt.c_str(),cloit->longOpt.c_str(), cloit->clType);
      }
      
      if(clOpts.nOpts == 0) 
      {
         return;
      }
      
      clOpts.parse(argc, argv, &nonOptions);
      
      for(it = targets.begin(); it != targets.end(); ++it)
      {
         if(clOpts.optSet(it->second.name)) 
         {
            std::vector<std::string> args;
   
            clOpts.getAll(args, it->second.name);
            it->second.values.insert( it->second.values.end(), args.begin(), args.end());
            it->second.set = true;
         }
      }      
      
   }

   ///Parse a config/ini file, updating the targets
   void readConfig(const std::string & fname)
   {
      iniFile iF;
      
      iF.parse(fname);
      
      targetIterator it;
      
      for(it = targets.begin(); it != targets.end(); ++it)
      {
         if(iF.count(it->second.section, it->second.keyword) > 0) 
         {
            it->second.values.push_back(iF(it->second.section, it->second.keyword));
            it->second.set = true;
         }
      }
   }
   
   /// Check if a target has been set by the configuration
   bool isSet(const std::string & name)
   {
      if(targets.count(name) == 0) return false;
      return targets[name].set;
   }

   /// Get the final value of the target, converted to the specified type
   template<typename typeT>
   typeT get(const std::string & name)
   {
      
      if(!isSet(name)) return false;
      
      return convertFromString<typeT>(targets[name].values.back());
   }
   
   template<typename typeT>
   std::vector<typeT> getVector(const std::string & name)
   {
      std::vector<typeT> v;

      std::string s = get<std::string>(name);
      
      int st;
      int com;

      st = 0;
      com = s.find(',', st);
   
      while(com != std::string::npos)
      {
         v.push_back( convertFromString<typeT>(s.substr(st, com-st)) );
         st = com + 1;
         com = s.find(',', st);
      }
      v.push_back( convertFromString<typeT>(s.substr(st, s.size()-st)));
   
      return v;
   }
   
   
   /// Get the i-th value of the target, converted to the specified config target
   template<typename typeT>
   typeT get(const std::string & name, int i)
   {
      if(!isSet(name)) return false;
      
      return convertFromString<typeT>(targets[name].values[i]);
   }
   
   /// Set a variable to the final value of a specified config target
   template<typename typeT>
   void set(typeT & var, const std::string & name)
   {
      if(!isSet(name)) return;
      
      var = convertFromString<typeT>(targets[name].values.back());
   }
   
   ///Get the number of different values set for the specified config target 
   int count(const std::string & name)
   {
      return targets[name].values.size();
   }
   
   
};


template<>
std::string appConfigurator::get<std::string>(const std::string & name)
{
   if(!isSet(name)) return "";
      
   return targets[name].values.back();
}



template<>
std::string appConfigurator::get<std::string>(const std::string & name, int i)
{
   if(!isSet(name)) return "";
      
   return targets[name].values[i];
}


   
} //namespace mx

#endif // __appConfigurator_hpp__