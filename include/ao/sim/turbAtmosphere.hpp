/** \file turbAtmosphere.hpp
  * \brief Declaration and definition of a turbulent atmosphere.
  * 
  * \author Jared R. Males (jaredmales@gmail.com)
  * 
  * \ingroup mxAO_sim_files
  *
  */

#ifndef __turbAtmosphere_hpp__
#define __turbAtmosphere_hpp__

#include <vector>
#include <iostream>

#include <mx/psdFilter.hpp>
#include <mx/psdUtils.hpp>
#include <mx/jinc.hpp>

#include "turbLayer.hpp"
#include "wavefront.hpp"

#ifdef DEBUG
#define BREAD_CRUMB std::cout << "DEBUG: " << __FILE__ << " " << __LINE__ << "\n"; 
#else
#define BREAD_CRUMB 
#endif

namespace mx
{
namespace AO
{
namespace sim
{

///A turbulent atmosphere simulator
/** \todo document this
  * \todo add facility for switching interpolators
  */   
template<typename _realT>
struct turbAtmosphere
{
   typedef _realT realT;
   typedef Eigen::Array<realT, -1, -1> arrayT;
    
   realT _pupD; ///<Size of the wavefront in meters.
   size_t _wfSz; ///<Size of the wavefront in pixels.
   size_t _buffSz; ///<Buffer to apply around wavefront for interpolation
   
   realT _lambda0; ///< Wavelength at which r_0 was measured.
   realT _lambda; ///< Desired wavelength of the turbulence.
   
   bool _subPiston; ///< Whether or not to subtract piston from the PSD
   bool _subTipTilt; ///< Whether or not to subtract tip/tilt from the PSD.
   
   std::vector<turbLayer<realT>> _layers; ///< Vector of turbulent layers.
   
   size_t _frames; ///< Length of the turbulence sequence.

   realT _F0Photons; ///< Photons per square meter from a 0 magnitude star at _lambda.
   
   realT _starMag; ///< Magnitude of the star being observed.
   
   realT _pixVal; ///< Nominal amplitude value for one pixel, has units of photons/pixel.
   
   arrayT * _pupil; ///< A pointer to the pupil mask.
   
   realT _timeStep; ///< Length of each iteration, in seconds.
   int _nWf; ///< Number of iterations which have occurred.
   
   std::string _dataDir; ///Specifies a path where phase screens are stored.
   
   bool _forceGen; ///Force generation of new screens if true.
   
   ///Default c'tor
   turbAtmosphere();

   ///Setup the overall atmosphere.
   /**
     * \param [in] pupD  New value of _pupD, the size of the wavefront in meters.
     * \param [in] wfSz  New value of _wfSz, the size of the wavefront in pixels.
     * \param [in] buffSz New value of _buffSz, the size of the interpolation buffer to use.
     * \param [in] lam0 New value of _lambda0, the wavelength at which r_0 is measured.
     * \param [in] lam New value of _lambda, the wavelength at which the phase is specified.
     * \param [in] subPist New value of _subPiston, whether or not piston is subtracted from the wavefront phase.
     * \param [in] subTip New value of _subTipTilt, whether or not tip/tilt is subtracted from wavefront phase.
     */ 
   int setup( realT pupD,
              size_t wfSz,
              size_t buffSz,
              realT lam0,
              realT lam,
              bool subPist,
              bool subTip );

               
   int setLayers( const std::vector<size_t> & scrnSz,
                  const std::vector<realT> & r0,
                  const std::vector<realT> & L0,
                  const std::vector<realT> & l0,
                  const std::vector<realT> & Cn2,
                  const std::vector<realT> & z,
                  const std::vector<realT> & windV,
                  const std::vector<realT> & windD );
  
   int setLayers( const size_t scrnSz,
                  const realT r0,
                  const realT L0,
                  const realT l0,
                  const std::vector<realT> & Cn2,
                  const std::vector<realT> & z,
                  const std::vector<realT> & windV,
                  const std::vector<realT> & windD );
   
   int genLayers();
   
   int shift( arrayT & phase,
              realT dt );
   
   int frames(int f);
   size_t frames();
   
   int wfPS(realT ps); ///< dummy function for simulatedAOSystem.  Does nothing.
   
   int F0Photons(realT f0);
   int starMag(realT mag);
   
   bool _loopClosed;
   
   void nextWF(wavefront<realT> & wf);
};   
   
template<typename realT>
turbAtmosphere<realT>::turbAtmosphere()
{
   _pupD = 0;
         
   _buffSz = 0;
   _wfSz = 0;
   
   _F0Photons = 0;
   _starMag = 0;
   
   _nWf = 0;
}

template<typename realT>
int turbAtmosphere<realT>::setup( realT pupD,
                                  size_t wfSz,
                                  size_t buffSz,
                                  realT lam0,
                                  realT lam,
                                  bool subPist,
                                  bool subTip )
{
   _pupD = pupD;
   _wfSz = wfSz;
   _buffSz = buffSz;
   
   _lambda0 = lam0;
   _lambda = lam;
   
   _subPiston = subPist;
   _subTipTilt = subTip;
   
   _forceGen = false;
   return 0;
}

template<typename realT>
int turbAtmosphere<realT>::setLayers( const std::vector<size_t> & scrnSz,
               const std::vector<realT> & r0,
               const std::vector<realT> & L0,
               const std::vector<realT> & l0,
               const std::vector<realT> & Cn2,
               const std::vector<realT> & z,
               const std::vector<realT> & windV,
               const std::vector<realT> & windD )
{
   
   size_t nLayers = scrnSz.size();
   
   if( r0.size() != nLayers)
   {
      mxError("turbAtmosphere::setLayers", MXE_INVALIDARG, "Size of r0 does not match.");
      return -1;
   }
   
   if( L0.size() != nLayers)
   {
      mxError("turbAtmosphere::setLayers", MXE_INVALIDARG, "Size of L0 does not match.");
      return -1;
   }
   
   if( l0.size() != nLayers)
   {
      mxError("turbAtmosphere::setLayers", MXE_INVALIDARG, "Size of l0 does not match.");
      return -1;
   }
   
   if( Cn2.size() != nLayers)
   {
      mxError("turbAtmosphere::setLayers", MXE_INVALIDARG, "Size of Cn2 does not match.");
      return -1;
   }
   
   if( z.size() != nLayers)
   {
      mxError("turbAtmosphere::setLayers", MXE_INVALIDARG, "Size of z does not match.");
      return -1;
   }
   
   if( windV.size() != nLayers)
   {
      mxError("turbAtmosphere::setLayers", MXE_INVALIDARG, "Size of windV does not match.");
      return -1;
   }
   
   if( windD.size() != nLayers)
   {
      mxError("turbAtmosphere::setLayers", MXE_INVALIDARG, "Size of windD does not match.");
      return -1;
   }
   
   _layers.clear();
   
   _layers.resize(nLayers);

   for(int i=0; i< nLayers; ++i)
   {
      _layers[i].setLayer( _wfSz, _buffSz, scrnSz[i], r0[i], L0[i], l0[i], _pupD, Cn2[i], z[i], windV[i], windD[i]);
   }
   
   return 0;
}

template<typename realT>
int turbAtmosphere<realT>::setLayers( const size_t scrnSz,
               const realT r0,
               const realT L0,
               const realT l0,
               const std::vector<realT> & Cn2,
               const std::vector<realT> & z,
               const std::vector<realT> & windV,
               const std::vector<realT> & windD )
{
   size_t n = Cn2.size();
   
   return setLayers( std::vector<size_t>(n, scrnSz), std::vector<realT>(n, r0), std::vector<realT>(n,L0), std::vector<realT>(n,l0),Cn2,z,windV,windD);
}

template<typename realT>
int turbAtmosphere<realT>::genLayers()
{
   if(_dataDir != "" && !_forceGen)
   {
      
      std::string fbase = _dataDir;
      fbase += "/";
      fbase += "layer_";
 
      mx::fitsFile<realT> ff;
      std::string fname;
      for(int i=0; i< _layers.size(); ++i)
      {
         fname = fbase + mx::convertToString<int>(i) + ".fits";
         ff.read(fname, _layers[i].phase);
      }
      
      return 0;
   }
   
   #pragma omp parallel num_threads(2)
   {
      arrayT psd;
      arrayT freq;
      mx::psdFilter<realT> filt;
   
      mx::normDistT<realT> normVar;
      normVar.seed();

      realT beta, L02, scrnSz, r0, L0, l0;
   
      realT sqrt_alpha = 0.5*11./3.;
   
      realT p;
      #pragma omp for 
      for(int i=0; i< _layers.size(); ++i)
      {
         std::cerr << "Generating layer " << i << " ";
         scrnSz = _layers[i]._scrnSz;
         
         r0 = _layers[i]._r0;
         L0 = _layers[i]._L0;
         l0 = _layers[i]._l0;
      
         std::cerr << scrnSz << " " << r0 << " " << L0 << " " << l0 << "\n";
         psd.resize(scrnSz, scrnSz);

         freq.resize(scrnSz, scrnSz);
         mx::frequency_grid(freq, _pupD/_wfSz);

   
         beta = 0.0218/pow( r0, 5./3.)/pow( _pupD/_wfSz,2) * pow(_lambda0/_lambda, 2);
      
         if(L0 > 0) L02 = 1.0/(L0*L0);
         else L02 = 0;

      
         for(int ii =0; ii < scrnSz; ++ii)
         {
            for(int jj=0; jj < scrnSz; ++jj)
            {
               if(freq(ii,jj) == 0 && L02 == 0)
               {
                  p = 0;
               }
               else
               {
                  p = beta / pow( pow(freq(ii,jj),2) + L02, sqrt_alpha);
                  if(l0 > 0 ) p *= exp(-1*pow( freq(ii,jj)*l0, 2)); 
            
                  realT Ppiston = 0;
                  realT Ptiptilt = 0;
                  if(_subPiston)
                  {
                     Ppiston = pow(2*mx::jinc(pi<realT>() * freq(ii,jj) * _pupD), 2);
                  }
            
                  if(_subTipTilt)
                  {               
                     Ptiptilt = pow(4*mx::jinc2(pi<realT>() * freq(ii,jj) * _pupD), 2);
                  }
            
                  p *= (1 - Ppiston - Ptiptilt);
            
               }
               psd(ii,jj) = p;
            
               _layers[i].phase(ii,jj) = normVar;
            }
         }   

         filt.psd(psd);
      
         filt(_layers[i].phase);
      }
   }//#pragma omp parallel
   
   
   if(_dataDir != "")
   {
      
      std::string fbase = _dataDir;
      fbase += "/";
      fbase += "layer_";
 
      mx::fitsFile<realT> ff;
      std::string fname;
      for(int i=0; i< _layers.size(); ++i)
      {
         fname = fbase + mx::convertToString<int>(i) + ".fits";
         ff.write(fname, _layers[i].phase);
      }
   }
   
   return 0;
}

template<typename realT>
int turbAtmosphere<realT>::shift( arrayT & phase,
                                  realT dt )
{
   phase.resize(_wfSz, _wfSz);
   phase.setZero();
   
   #pragma omp parallel for
   for(int j=0; j< _layers.size(); ++j)
   {
      _layers[j].shift( dt );

      //This is faster than a separate loop by 5-10%
      #pragma omp critical
      phase += sqrt( _layers[j]._Cn2 ) * _layers[j].shiftPhase.block(_buffSz, _buffSz, _wfSz, _wfSz);
   }
   
   return 0;
}
 
template<typename realT> 
int turbAtmosphere<realT>::frames(int f)
{
   _frames = f;
}

template<typename realT>
size_t turbAtmosphere<realT>::frames()
{
   return _frames;
}
   
template<typename realT>
int turbAtmosphere<realT>::wfPS(realT ps)
{
   return 0;
}
   
template<typename realT>   
int turbAtmosphere<realT>::F0Photons(realT f0)
{
   _F0Photons = f0;
   _pixVal = _pixVal = sqrt(_F0Photons)*pow(10., -0.2*_starMag)*(_pupD/_wfSz);
}

template<typename realT>
int turbAtmosphere<realT>::starMag(realT mag)
{
   _starMag = mag;
   _pixVal = _pixVal = sqrt(_F0Photons)*pow(10., -0.2*_starMag)*(_pupD/_wfSz);
}

template<typename realT>
void turbAtmosphere<realT>::nextWF(wavefront<realT> & wf)
{
      
   int Npix = _pupil->sum();
      
   shift( wf.phase, _nWf * _timeStep);
   ++_nWf;
      
   wf.phase = (wf.phase - (wf.phase* (*_pupil)).sum()/Npix)* (*_pupil);

   wf.amplitude = _pixVal*(*_pupil);
}

} //namespace sim
} //namespace AO
} //namespace mx
#endif //__turbAtmosphere_hpp__