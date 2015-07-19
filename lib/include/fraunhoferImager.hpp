/** \file fraunhoferImager.hpp
  * \brief Declares and defines a class for Fraunhofer propagation of optical wavefronts
  * \ingroup imaging
  * \author Jared R. Males (jaredmales@gmail.com)
  *
  */


#ifndef __fraunhoferImager_hpp__
#define __fraunhoferImager_hpp__

#include "imagingUtils.hpp"
#include "fft.hpp"

namespace mx
{
   
/// Class to perform Fraunhofer propagation between pupil and focal planes
/** This class uses the FFT to propagate between planes, and normalizes so that flux
  * is conserved.  For propagation from pupil to focal plane, the pupil wavefront is tilted so that 
  * the focal-plane image is centered at the geometric center of the array.  After propagation from 
  * focal plane to pupil plane, the pupil plane wavefront is un-tilted to restore the 
  * pupil to its original position.  
  * 
  * \tparam arithT is the arithmetic type
  * 
  * \ingroup imaging
  */ 
template<typename arithT>
class fraunhoferImager 
{
   
public:
   ///The complex data type
   typedef std::complex<arithT> complexT;
   
   ///The wavefront data type
   typedef Eigen::Array<std::complex<arithT>, Eigen::Dynamic, Eigen::Dynamic> wavefrontT;
   
protected:

   ///The size of the wavefront in pixels
   int wavefrontSizePixels;

   ///The physical size of the wavefront
   /** This is used for calculating the plate scale
     */
   arithT wavefrontSizePhysical;

   arithT xcen; ///<x-coordinate of focal plane center, in pixels
   arithT ycen; ///<x-coordinate of focal plane center, in pixels
     
   ///Phase screen for tilting the pupil plane so that the focal plane image is centered.  
   wavefrontT centerFocal;  
   
   ///Phase screen for un-tilting the pupil plane after propagating from a centered focal plane.
   wavefrontT centerPupil;

   ///Initialize members
   void initialize()
   {
      wavefrontSizePixels = 0;
      wavefrontSizePhysical = 0;
      
      xcen = 0;
      ycen = 0;
   }
   
public:
   
   ///Constructor
   fraunhoferImager()
   {
      initialize();
   }
   
   ///Propagate the wavefront from the pupil plane to the focal plane
   /** The pupil plane wavefront (complexPupil) is multiplied by a tilt to place the
     * image in the geometric center of the focal plane. 
     * 
     * \param [out] complexFocal the focal plane wavefront.  Must be pre-allocated to same size as complexPupil.
     * \param [in] complexPupil the pupil plane wavefront. Modified due to application of centering tilt.
     * 
     */ 
   void propagatePupilToFocal(wavefrontT & complexFocal, wavefrontT & complexPupil)
   {
      //First setup the tilt screens (does nothing if there's not change in size)
      setWavefrontSizePixels(complexPupil.rows());
      
      //DFT normalization, sqrt(2) for complex number
      arithT norm = wavefrontSizePixels/sqrt(2.);
      
      //Apply the centering shift -- this adjusts by 0.5 pixels
      complexPupil *= centerFocal;
            
      fft(complexFocal, complexPupil);
      
      //Normalize
      complexFocal = complexFocal / complexT(norm, norm);
      
   }
   
   ///Propagate the wavefront from Focal plane to Pupil plane
   /** 
     * After the fourier transform, the output pupil plane wavefront is de-tilted, restoring it
     * to the state prior to calling \ref propagatePupilToFocal
     * 
     * \param complexPupil [output] the pupil plane wavefront. Must be pre-allocated to same size as complexFocal.
     * \param complexFocal [input] the focal plane wavefront.
     * 
     */ 
   void propagateFocalToPupil(wavefrontT & complexPupil, wavefrontT & complexFocal)
   {
      //DFT normalization, sqrt(2) for complex number
      arithT norm = wavefrontSizePixels/sqrt(2.);
      
      fft(complexPupil, complexFocal, FFTW_BACKWARD);
      
      //Unshift the wavefront
      complexPupil *= centerPupil;
      
      complexPupil /= complexT(norm, norm);
   }
   
   /// Set the physical size of the wavefront
   /**
     * \param wfsPhys is the new size of the wavefront
     */ 
   void setWavefrontSizePhysical(int wfsPhys)
   {      
      wavefrontSizePhysical = wfsPhys;
   }

   ///Set the size of the wavefront, in pixels
   /** Checks if the size changes, does nothing if no change.  Otherwise,  calls
     * \ref makeShiftPhase to pre-calculate the tilt arrays.
     *
     * \param wfsPix [input] the desired new size of the wavefront
     */ 
   void setWavefrontSizePixels(int wfsPix)
   {
      //If no change in size, do nothing
      if(wfsPix == centerFocal.rows()) return;
            
      wavefrontSizePixels = wfsPix;
      
      xcen = 0.5*(wfsPix - 1.0);
      ycen = 0.5*(wfsPix - 1.0);
      
      makeShiftPhase();
   }   
   
protected:
   
   ///Calculate the complex tilt arrays for centering the wavefronts
   /**
     */
   void makeShiftPhase()
   {      
      //Resize the center phases
      centerFocal.resize(wavefrontSizePixels, wavefrontSizePixels);
      centerPupil.resize(wavefrontSizePixels, wavefrontSizePixels);

      //Shift by 0.5 pixels
      arithT arg = -1.0*D2PI*0.5*wavefrontSizePixels/(wavefrontSizePixels-1);
  
      for(int ii=0; ii < wavefrontSizePixels; ++ii)
      {
         for(int jj=0; jj < wavefrontSizePixels; ++jj)
         {     
            centerFocal(ii,jj) = exp(complexT(0.,arg*((ii-xcen)+(jj-ycen))));
            centerPupil(ii,jj) = exp(complexT(0., 0.5*DPI - arg*((ii-xcen)+(jj-ycen))));
         }
      }
   }

   
};//class fraunhoferImager

} //namespace mx

#endif //__fraunhoferImager_hpp__

