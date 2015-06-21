

#ifndef __iniFile_hpp__
#define __iniFile_hpp__

#include <unordered_map>

#include "ini.hpp"


struct iniFile 
{
   typedef std::unordered_map<std::string, std::string> nameMapT;

   nameMapT names;

   std::string makeKey(const std::string & section, const std::string & name)
   {
      return section + "=" + name;
   }

   void parse(const std::string fname)
   {
      ini_parse(fname.c_str(), handler, this);
   }

   int insert(const char *section, const char *name, const char * value)
   {    
      std::string nkey = makeKey(section, name);

      if (names[nkey].size() > 0)
      {
        names[nkey] += "\n";
      }

      names[nkey] += value;
      return 1;
   }

   static int handler(void* user, const char* section, const char* name,
                            const char* value)
   {
      iniFile * iF = (iniFile *) user;
      return iF->insert(section, name, value);
   }
   
   int count(const std::string &section, const std::string & name)
   {
      return names.count(makeKey(section, name));
   }
   
   std::string operator()(const std::string &section, const std::string & name)
   {
      std::string key = makeKey(section, name); 
      if(names.count(key) > 0)
      {
         return names[key];
      }
      else
      {
         return std::string("");
      }
   }
   
   std::string operator()(const std::string & name)
   {
      return names[makeKey("", name)];
   }
   
   
};


#endif
