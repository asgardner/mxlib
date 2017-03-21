/** \file turbLayer.hpp
  * \brief Declaration and definition of a turbulence layer.
  * 
  * \author Jared R. Males (jaredmales@gmail.com)
  * 
  * \ingroup mxAO_sim_files
  *
  */

#ifndef __turbLayer_hpp__
#define __turbLayer_hpp__

#include <Eigen/Dense>

#include <mx/randomT.hpp>
#include <mx/randomSeed.hpp>

#include <mx/imageTransforms.hpp>


namespace mx
{
namespace AO
{
namespace sim
{
   
///Simulation of a single turbulent layer
/** \todo document this
  * \todo add facility for changing interpolator
  */
template<typename _realT>
struct turbLayer
{
   typedef _realT realT;
   typedef Eigen::Array<realT, -1, -1> arrayT;
   
   size_t _wfSz;
   size_t _buffSz;
   size_t _scrnSz;
   
   realT _r0;
   realT _L0;
   realT _l0;
   realT _pupD;
   realT _Cn2;
   realT _z;
   realT _windV;
   realT _windD;
   
   realT _dx;
   realT _dy;

   int _last_wdx;
   int _last_wdy;

   arrayT phase;
   arrayT shiftPhaseWP;
   arrayT shiftPhase;

   mx::uniDistT<realT> uniVar; ///< Uniform deviate, used in shiftRandom.
   
   turbLayer();
   
   void setLayer( int wfSz,
                  int buffSz,
                  int scrnSz,
                  realT r0,
                  realT L0,
                  realT l0, 
                  realT pupD,
                  realT Cn2,
                  realT z,
                  realT windV, 
                  realT windD );
   
   void alloc();

   ///Shift to a timestep.
   /**
     * \param [in] dt is the new timestep.
     */ 
   void shift( realT dt );
   
   
   ///Seed the uniform deviation.  Call this if you intend to use shiftRandom.
   /** This only needs to be called once.
     */
   void initRandom();
   
   ///Shift by a random amount using the uniform distribution
   /** Call initRandom() once before calling this method.
     * 
     * \param [in] nofract if true then the fractional part is ignored, and only a whole-pixel shift is executed.  Default=false
     */ 
   void shiftRandom( bool nofract=false );
   
   
};

template<typename realT>
turbLayer<realT>::turbLayer()
{
}

template<typename realT>
void turbLayer<realT>::setLayer( int wfSz,
               int buffSz,
               int scrnSz,
               realT r0,
               realT L0,
               realT l0, 
               realT pupD,
               realT Cn2,
               realT z,
               realT windV, 
               realT windD )
{
   
   _wfSz = wfSz;
   _buffSz = buffSz;
   _scrnSz = scrnSz;
   
   _r0 = r0;
   _L0 = L0;
   _l0 = l0;
   
   _pupD = pupD;
   
   _Cn2 = Cn2;
   _z = z;
   _windV = windV;
   _windD = windD;
   
   _dx = _windV*cos(_windD)/(_pupD/_wfSz);
   _dy = _windV*sin(_windD)/(_pupD/_wfSz);
   
   _last_wdx = _scrnSz + 1;
   _last_wdy = _scrnSz + 1;

   alloc();
}

template<typename realT>
void turbLayer<realT>::alloc()
{      
   phase.resize(_scrnSz, _scrnSz);
   shiftPhaseWP.resize( _wfSz+2*_buffSz, _wfSz+2*_buffSz);
   shiftPhase.resize( _wfSz+2*_buffSz, _wfSz+2*_buffSz);
   
   //fft.plan( _wfSz+2*_buffSz, _wfSz+2*_buffSz, MXFFT_FORWARD, true);
   //fftR.plan( _wfSz+2*_buffSz, _wfSz+2*_buffSz, MXFFT_BACKWARD, true);
}

template<typename realT>
void turbLayer<realT>::shift( realT dt )
{
   int wdx, wdy;
   realT ddx, ddy;

   ddx = _dx*dt;
   ddy = _dy*dt;
   
   wdx = (int) trunc(ddx);
   ddx -= wdx;
   wdy = (int) trunc(ddy);
   ddy -= wdy;
   
   wdx %= _scrnSz;
   wdy %= _scrnSz;

   //Check for a new whole-pixel shift
   if(wdx != _last_wdx || wdy != _last_wdy)
   {
      //Need a whole pixel shift
      mx::imageShiftWP(shiftPhaseWP, phase, wdx, wdy);
   }
   
   //Do the sub-pixel shift      
   mx::imageShift( shiftPhase, shiftPhaseWP, ddx, ddy, mx::cubicConvolTransform<realT>(-0.5));
   
   _last_wdx = wdx;
   _last_wdy = wdy;
   
}

template<typename realT>
void turbLayer<realT>::initRandom()
{
   uniVar.seed();
}

template<typename realT>
void turbLayer<realT>::shiftRandom( bool nofract )
{
   int wdx, wdy;
   realT ddx, ddy;


   ddx = uniVar * (_scrnSz);
   ddy = uniVar * (_scrnSz);
   
   wdx = (int) trunc(ddx);
   ddx -= wdx;
   wdy = (int) trunc(ddy);
   ddy -= wdy;
   
   if(nofract)
   {  
      ddx=0;
      ddy=0;
   }
   
   mx::imageShiftWP(shiftPhaseWP, phase, wdx, wdy);
   
   if(ddx !=0 || ddy != 0)
   {
      mx::imageShift( shiftPhase, shiftPhaseWP, ddx, ddy, mx::cubicConvolTransform<realT>(-0.5));
   }
   else
   {
      shiftPhase = shiftPhaseWP;
   }
}

} //namespace sim
} //namespace AO
} //namespace mx
#endif //__turbLayer_hpp__