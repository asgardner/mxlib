/** \file ADIobservation.hpp
  * \author Jared R. Males
  * \brief Defines the ADI high contrast imaging data type.
  * \ingroup hc_imaging
  *
  */

#include "HCIobservation.hpp"
#include "fitsHeader.hpp"

#ifndef __ADIobservation_hpp__
#define __ADIobservation_hpp__

namespace mx
{
   
template<typename _floatT>
struct derotVisAO
{
   typedef  _floatT floatT;

   ///Vector of keywords to extract from the fits headers
   std::vector<std::string> keywords;
   
   ///Vector(s) to hold the keyword values
   std::vector<floatT> rotoff;
   
   
   ///Constructor should populate keywords
   derotVisAO()
   {
      keywords.push_back("ROTOFF");
   }
   
   ///Method called by DIobservation to get keyword-values
   void extractKeywords(std::vector<fitsHeader> & heads)
   {
      rotoff = headersToValues<floatT>(heads, "ROTOFF");
   }
   
   ///Calculate the derotation angle for a given image number
   floatT derotAngle(size_t imno) const
   {
      return DTOR(rotoff[imno]+90-0.6);
   }
};

template<typename _floatT>
struct derotClio
{
   typedef  _floatT floatT;

   ///Vector of keywords to extract from the fits headers
   std::vector<std::string> keywords;
   
   ///Vector(s) to hold the keyword values
   std::vector<floatT> rotoff;
   
   
   ///Constructor should populate keywords
   derotClio()
   {
      keywords.push_back("ROTOFF");
   }
   
   ///Method called by DIobservation to get keyword-values
   void extractKeywords(std::vector<fitsHeader> & heads)
   {
      rotoff = headersToValues<floatT>(heads, "ROTOFF");
   }
   
   ///Calculate the derotation angle for a given image number
   floatT derotAngle(size_t imno) const
   {
      return DTOR(rotoff[imno]-180-1.8);
   }
};

template<typename _floatT>
struct derotODI
{
   typedef  _floatT floatT;

   ///Vector of keywords to extract from the fits headers
   std::vector<std::string> keywords;
   
   ///Vector(s) to hold the keyword values
   std::vector<floatT> dateobs;
   
   ///The period of the orbit
   _floatT period;
   
   ///Constructor should populate keywords
   derotODI()
   {
      period = 365.25;
      keywords.push_back("DATEOBS");
   }
   
   ///Method called by DIobservation to get keyword-values
   void extractKeywords(std::vector<fitsHeader> & heads)
   {
      dateobs = headersToValues<floatT>(heads, "DATEOBS");
   }
   
   ///Calculate the derotation angle for a given image number
   floatT derotAngle(size_t imno) const
   {
      return D2PI-(fmod(dateobs[imno], period)/period)*D2PI;
   }
};

/** \addtogroup hc_imaging
 * @{
 */

///Process an angular differential imaging (ADI) observation
/** Angular differential imaging (ADI) uses sky rotation to differentiate real objects from
  * speckles.
  * 
  * \tparam floatT is the floating point type in which to do calculations
  * 
  * \tparam _derotFunctObj 
  * \parblock 
  * is the derotation functor with the following minimum interface: 
  * \code
  * template<typename _floatT>
  * struct derotF
  * {
  *    typedef  _floatT floatT;
  * 
  *    //Vector of keywords to extract from the fits headers
  *    std::vector<std::string> keywords;
  *    
  *    //Vector(s) to hold the keyword values
  *    std::vector<floatT> keyValue1;
  *    
  *    
  *    //Constructor should populate keywords
  *    derotVisAO()
  *    {
  *       keywords.push_back("KEYWORD1");
  *    }
  *    
  *    //Method called by HCIobservation to get keyword-values
  *    void extractKeywords(vector<fitsHeader> & heads)
  *    {
  *       keyValue1 = headersToValues<float>(heads, "KEYWORD1");
  *    }
  *    
  *    //Calculate the derotation angle for a given image number
  *    floatT derotAngle(size_t imno) const
  *    {
  *       return DTOR(keyValue1[imno]+90-0.6);
  *    }
  * };
  * \endcode
  * \endparblock
  */
template<typename _floatT, class _derotFunctObj>
struct ADIobservation : public HCIobservation<_floatT>
{
   typedef _floatT floatT;
   typedef _derotFunctObj derotFunctObj;
   typedef Array<floatT, Eigen::Dynamic, Eigen::Dynamic> eigenImageT;
   
   derotFunctObj derotF;
   //vector<floatT> derot;

   bool doDerotate;
   
   ADIobservation();
   
   ADIobservation( const std::string & fileListFile) ;
                   
   ADIobservation( const std::string & dir, 
                   const std::string & prefix, 
                   const std::string ext = ".fits") ;

   void initialize();
   
   ///Read in the files
   /** First sets up the keywords, then calls HCIobservation readFiles
     */
   void readFiles();
   
   ///Post read actions, including fake injection
   virtual void postReadFiles();
   
   /** \name Fake Planets
     * @{ 
     */
//   int doFake; ///<Flag controlling whether or not fake planets are injected
   std::string fakeFileName; ///<FITS file containing the fake planet PSF to inject
//   bool doFakeScale; ///<Flag controlling whether or not a separate scale is used at each point in time
   std::string fakeScaleFileName; ///< One-column text file containing a scale factor for each point in time.
   
   std::vector<floatT> fakeSep; ///< Separation(s) of the fake planet(s)
   std::vector<floatT> fakePA; ///< Position angles(s) of the fake planet(s)
   std::vector<floatT> fakeContrast; ///< Contrast(s) of the fake planet(s)
   
   ///Inect the fake plants
   void injectFake();
   
   /// @}
   
   void fitsHeader(fitsHeader * head);
   
   ///De-rotate the PSF subtracted images
   void derotate();

   double t_fake_begin;
   double t_fake_end;
   
   double t_derotate_begin;
   double t_derotate_end;
   
};

template<typename _floatT, class _derotFunctObj>
ADIobservation<_floatT, _derotFunctObj>::ADIobservation()
{
   initialize();
}

template<typename _floatT, class _derotFunctObj>
ADIobservation<_floatT, _derotFunctObj>::ADIobservation( const std::string & fileListFile) : HCIobservation<floatT>(fileListFile)
{
   initialize();
}

template<typename _floatT, class _derotFunctObj>
ADIobservation<_floatT, _derotFunctObj>::ADIobservation( const std::string & dir, 
                                                         const std::string & prefix, 
                                                         const std::string ext) : HCIobservation<floatT>(dir,prefix,ext)
{
   initialize();
}
template<typename _floatT, class _derotFunctObj>
void ADIobservation<_floatT, _derotFunctObj>::initialize()
{
   doDerotate = true;
//   doFake = 0;
//   doFakeScale = 0;
   
   t_fake_begin = 0;
   t_fake_end = 0;
   
   t_derotate_begin = 0;
   t_derotate_end = 0;
}
template<typename _floatT, class _derotFunctObj>
void ADIobservation<_floatT, _derotFunctObj>::readFiles()
{      
   this->keywords.clear();
   for(int i=0;i<derotF.keywords.size();++i)
   {
      this->keywords.push_back(derotF.keywords[i]);
   }
   
   HCIobservation<floatT>::readFiles();
   
   derotF.extractKeywords(this->heads);
}

template<typename _floatT, class _derotFunctObj>
void ADIobservation<_floatT, _derotFunctObj>::postReadFiles()
{
   derotF.extractKeywords(this->heads);
   
   if(fakeFileName != "") injectFake();
}

template<typename _floatT, class _derotFunctObj>
void ADIobservation<_floatT, _derotFunctObj>::injectFake()
{
   std::cerr << "injecting fake planets\n";

   t_fake_begin = get_curr_time();
   
   typedef Eigen::Array<floatT, Eigen::Dynamic, Eigen::Dynamic> imT;
   imT fakePSF;
   fitsFile<floatT> ff;
   std::ifstream scaleFin; //for reading the scale file.
      
   ff.read(fakeFileName, fakePSF);

   //Check for correct sizing
   if( (fakePSF.rows() < this->imc.rows() && fakePSF.cols() >= this->imc.cols()) || 
                        (fakePSF.rows() >= this->imc.rows() && fakePSF.cols() < this->imc.cols()))
   {
      throw mxException("mxlib:high contrast imaging", -1, "image wrong size",  __FILE__, __LINE__, "fake PSF has different dimensions and can't be sized properly");
   }
   
   //Check if fake needs to be padded out
   if(fakePSF.rows() < this->imc.rows() && fakePSF.cols() < this->imc.cols())
   {
      imT pfake(this->imc.rows(), this->imc.cols());
      padImage(pfake, fakePSF, this->imc.rows(), this->imc.cols());
      fakePSF = pfake;
   }
   
   //Check if fake needs to be cut down
   if(fakePSF.rows() > this->imc.rows() && fakePSF.cols() > this->imc.cols())
   {
      imT cfake(this->imc.rows(), this->imc.cols());
      cutImage(cfake, fakePSF, this->imc.rows(), this->imc.cols());
      fakePSF = cfake;
   }
   
   
   //allocate shifted fake psf
   imT shiftFake(fakePSF.rows(), fakePSF.cols());
   
   floatT ang, dx, dy;

   //Fake Scale -- default to 1, read from file otherwise
   std::vector<floatT> fakeScale(this->imc.planes(), 1.0);
   if(fakeScaleFileName != "")
   {
      
      std::vector<std::string> sfileNames;
      std::vector<floatT> imS;
      
      //Read the quality file and load it into a map
      readColumns(fakeScaleFileName, sfileNames, imS);
      
      std::map<std::string, floatT> scales;     
      for(int i=0;i<sfileNames.size();++i) scales[sfileNames[i]] = imS[i];
      
      for(int i=0; i<this->fileList.size(); ++i)
      {
         if(scales.count(basename(this->fileList[i].c_str())) > 0)
         {
            fakeScale[i] = scales[basename(this->fileList[i].c_str())];
         }
         else
         {
            std::cerr << "File name not found in fakeScaleFile:\n";
            std::cerr << basename(this->fileList[i].c_str()) << "\n";
            exit(-1);
         }
      }

//       scaleFin.open(fakeScaleFileName.c_str());
//       for(int i=0; i<this->imc.planes(); ++i)
//       {
//          scaleFin >> fakeScale[i];
//       }
//       scaleFin.close();
   }
      
      
   for(int i=0; i<this->imc.planes(); ++i)
   {
      for(int j=0;j<fakeSep.size(); ++j)
      {
         ang = DTOR(-fakePA[j]) + derotF.derotAngle(i);
      
         dx = fakeSep[j] * sin(ang);
         dy = fakeSep[j] * cos(ang);
               
         imageShift(shiftFake, fakePSF, dx, dy, cubicConvolTransform<floatT>());
   
         this->imc.image(i) = this->imc.image(i) + shiftFake*fakeScale[i]*fakeContrast[j];
      }
   }
   
   
   std::cerr << "fake injected\n";
   
   t_fake_end = get_curr_time();
}


template<typename _floatT, class _derotFunctObj>
void ADIobservation<_floatT, _derotFunctObj>::derotate()
{
   t_derotate_begin = get_curr_time();
   
   //On magaoarx it doesn't seem worth it to use more than 4 threads
   #pragma omp parallel num_threads(4)
   {
      eigenImageT rotim;
      floatT derot;
      
      #pragma omp for schedule(static, 1)
      for(int n=0; n<this->psfsub.size(); ++n)
      {
         for(int i=0; i<this->psfsub[n].planes();++i)
         {
            derot = derotF.derotAngle(i);
            if(derot != 0) 
            {
               imageRotate(rotim, this->psfsub[n].image(i), derot, cubicConvolTransform<floatT>());
               this->psfsub[n].image(i) = rotim;
            }
         }
      }
   }
   
   t_derotate_end = get_curr_time();
}


template<typename _floatT, class _derotFunctObj>
void ADIobservation<_floatT, _derotFunctObj>::fitsHeader(mx::fitsHeader * head)
{
   if(head == 0) return;
   
   head->append("", fitsCommentType(), "----------------------------------------");
   head->append("", fitsCommentType(), "mx::ADIobservation parameters:");
   head->append("", fitsCommentType(), "----------------------------------------");

   head->append("FAKEFILE", fakeFileName, "name of fake planet PSF file");
   head->append("FAKESCFL", fakeScaleFileName, "name of fake planet scale file name");
   
   std::stringstream str;
   for(int nm=0;nm < fakeSep.size()-1; ++nm) str << fakeSep[nm] << ",";
   str << fakeSep[fakeSep.size()-1];      
   head->append<char *>("FAKESEP", (char *)str.str().c_str(), "separation of fake planets");
   
   str.str("");
   for(int nm=0;nm < fakePA.size()-1; ++nm) str << fakePA[nm] << ",";
   str << fakePA[fakePA.size()-1];      
   head->append<char *>("FAKEPA", (char *)str.str().c_str(), "PA of fake planets");
   
   str.str("");
   for(int nm=0;nm < fakeContrast.size()-1; ++nm) str << fakeContrast[nm] << ",";
   str << fakeContrast[fakeContrast.size()-1];      
   head->append<char *>("FAKECONT", (char *)str.str().c_str(), "Contrast of fake planets");
}

///@}

} //namespace mx

#endif //__ADIobservation_hpp__


