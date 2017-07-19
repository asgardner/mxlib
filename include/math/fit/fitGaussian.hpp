/** \file fitGaussian.hpp
 * \author Jared R. Males
 * \brief Tools for fitting Gaussians to data.
 * \ingroup fitting_files
 *
 */

#ifndef fitGaussian_hpp
#define fitGaussian_hpp

#include "levmarInterface.hpp"
#include "../func/gaussian.hpp"

namespace mx
{
namespace math
{
namespace fit
{

/** \defgroup gaussian_peak_fit Gaussians
  * \brief Fitting Gaussians to data.
  * 
  * The Gaussian function is fit to data.  
  * 
  * \ingroup peak_fit 
  */
   
//forward
template<typename _realT>
struct gaussian2D_sym_fitter;

//forward
template<typename _realT>
struct gaussian2D_gen_fitter;

///Class to manage fitting a 2D Gaussian to data via the \ref levmarInterface
/** In addition to the requirements on fitterT specified by \ref levmarInterface
  * this class also requires the following in fitterT
  * \code
  * static const int nparams = 7; //where the number 7 is replaced by the number of parameters that fitterT expects to fit. 
  * 
  * void paramNormalizer( realT *, int); //A function which converts from input parameters to fitting parameters.  May do nothing.
  * 
  * \endcode
  * 
  *
  * \tparam fitterT a type meeting the above requirements.
  *
  * \ingroup gaussian_peak_fit
  *
  */
template<typename fitterT>
class fitGaussian2D : public levmarInterface<fitterT>
{
   
public:
   
   typedef typename fitterT::realT realT;

   static const int nparams = fitterT::nparams;

   array2Fit<realT> arr;
   
   void initialize()
   {
      this->allocate_params(nparams);
      this->adata = &arr;      
   }
   
   fitGaussian2D()
   {
      initialize();
   }
      
   ~fitGaussian2D()
   {
   }
   
   ///Set the initial guess for a symmetric Gaussian.
   /** Also works for the general case, setting the same width in both directions.
     */
   void setGuess( realT G0,    ///< [in] the constant background level
                  realT A,     ///< [in] the peak scaling
                  realT x0,     ///< [in] the center x-coordinate
                  realT y0,     ///< [in] the center y-coordinate
                  realT sigma   ///< [in] the width parameter
                )
   {
      static_assert(nparams > 4, "fitGaussian2D: not enough parameters for this setGuess call (need at least 5).");
      
      this->p[0] = G0;
      this->p[1] = A;
      this->p[2] = x0;
      this->p[3] = y0;      
      this->p[4] = sigma;
      
      if(nparams == 7)
      {
         this->p[5] = sigma;
         this->p[6] = 0.0;
      }
   }
   
   ///Set the initial guess for the general Gaussian.
   void setGuess( realT G0,    ///< [in] the constant background level
                  realT A,     ///< [in] the peak scaling
                  realT x0,     ///< [in] the center x-coordinate
                  realT y0,     ///< [in] the center y-coordinate
                  realT sigma_x,   ///< [in] the width parameter in the rotated x-direction (the long axis) 
                  realT sigma_y,   ///< [in] the width parameter in the rotated y-direction
                  realT theta    ///< [in] the angle of the long axis (always sigma_x)
                )
   {
      static_assert(nparams > 6, "fitGaussian2D: not enough parameters for this setGuess call (need at least 7).");
      
      
      this->p[0] = G0;
      this->p[1] = A;
      this->p[2] = x0;
      this->p[3] = y0;      
      this->p[4] = sigma_x;
      this->p[5] = sigma_y;
      this->p[6] = theta;
   }
   
   ///Set the data aray.
   void setArray( realT *data, 
                  int nx, 
                  int ny
                )
   {
      arr.data = data;
      arr.nx = nx;
      arr.ny = ny;
      
      this->n = nx*ny;
      
   }
   
   ///Do the fit.
   int fit()
   {
      fitterT fitter;
      fitter.paramNormalizer(this->p, 1);
      
      levmarInterface<fitterT>::fit();
      
      fitter.paramNormalizer(this->p, -1);
      fitter.paramNormalizer(this->init_p, -1); //The normalized version is stored, so fix it before possible output.
   }
     
   ///Get the current value of G0, the constant.
   /**
     * \returns the current value of G0, which is p[0].
     */ 
   realT G0()
   {
      return this->p[0];
   }

   ///Get the peak scaling.
   realT A()
   {
      return this->p[1];
   }

   ///Get the center x-coordinate
   realT x0()
   {
      return this->p[2];
   }
   
   ///Get the center y-coordinate
   realT y0()
   {
      return this->p[3];
   }
   
   ///Return the width parameter
   /** As described for the symmetric Gaussian.
     *
     * For the general Gaussian, this returns \f$ \sigma = \sqrt{ \sigma_x^2 + \sigma_y^2} \f$.
     */ 
   realT sigma()
   {
      if(nparams == 5)
      {
         return this->p[4];
      }
      else
      {
         return sqrt( pow(this->p[4],2) + pow(this->p[5],2));
      }
   }
   
   ///Return the width parameter on the long axis.
   realT sigma_x()
   {
      static_assert( nparams==7 , "No sigma_x in symmetric Gaussian.");
      return this->p[4];
   }
   
   ///Return the width parameter on the short axis.
   realT sigma_y()
   {
      static_assert( nparams==7 , "No sigma_y in symmetric Gaussian.");
      return this->p[5];
   }
   
   ///Return the orientation of the long axis.
   realT theta()
   {
      static_assert( nparams==7 , "No theta in symmetric Gaussian.");
      return this->p[6];
   }
   
};


///\ref levmarInterface fitter structure for the symmetric Gaussian.
/** \ingroup gaussian_peak_fit
  *
  */
template<typename _realT>
struct gaussian2D_sym_fitter
{
   typedef _realT realT;
   
   static const int nparams = 5;
   
   static void func(realT *p, realT *hx, int m, int n, void *adata)
   {
      array2Fit<realT> * arr = (array2Fit<realT> *) adata;
   
      size_t idx_mat, idx_dat;

      idx_dat = 0;
   
      for(int i=0;i<arr->nx; i++)
      {
         for(int j=0;j<arr->ny;j++)
         { 
            idx_mat = i+j*arr->nx;
   
            hx[idx_dat] = func::gaussian2D<realT>(i,j,p[0],p[1], p[2], p[3], p[4]) - arr->data[idx_mat];
            
            hx[idx_dat] *= fabs(arr->data[idx_mat]);
            
            idx_dat++;
         }
      }
   }
   
   ///Does nothing in this case.
   void paramNormalizer(realT * p, int dir)
   {
      return;
   }
};

///\ref levmarInterface fitter structure for the general elliptical Gaussian.
/** \ingroup gaussian_peak_fit
  *
  */
template<typename _realT>
struct gaussian2D_gen_fitter
{
   typedef _realT realT;
   
   static const int nparams = 7;
   
   //typedef bool hasJacobian; //The Jacobian may not be correct, possibly due to poss-def problem.
   
   static void func(realT *p, realT *hx, int m, int n, void *adata)
   {
      array2Fit<realT> * arr = (array2Fit<realT> *) adata;
   
      size_t idx_mat, idx_dat;

      //Check for positive-definiteness of {{a b}{b c}}
      if( p[4]*p[6] - p[5]*p[5] <= 0 || p[4] <= 0 || p[6] <= 0 || p[4]+p[6] <= 2*fabs(p[6]))
      {
         idx_dat = 0;
         //If it's not positive-definite, then we just fill in with the value of the image itself.
         for(int i=0;i<arr->nx; ++i)
         {
            for(int j=0;j<arr->ny; ++j)
            { 
               idx_mat = i+j*arr->nx;
   
               hx[idx_dat] = arr->data[idx_mat];
                        
               ++idx_dat;
            }
         }         
         return;
      }
      
      //If positive-definite, now actually calculate
      idx_dat = 0;
   
      for(int i=0;i<arr->nx; ++i)
      {
         for(int j=0;j<arr->ny; ++j)
         { 
            idx_mat = i+j*arr->nx;
   
            hx[idx_dat] = func::gaussian2D<realT>(i,j, p[0], p[1], p[2], p[3], p[4], p[5], p[6]) - arr->data[idx_mat];
                        
            hx[idx_dat] *= fabs(arr->data[idx_mat]);
            
            ++idx_dat;
         }
      }
   }
   
   static void jacf(realT *p, realT *jacob, int m, int n, void *adata)
   {
      array2Fit<realT> * arr = (array2Fit<realT> *) adata;
   
      size_t idx_mat, idx_dat;

      realT j_tmp[7];
      
      //Check for positive-definiteness of {{a b}{b c}}
      if( p[4]*p[6] - p[5]*p[5] <= 0)
      {
         idx_dat = 0;
         //If it's not positive-definite, then we just fill in with 0s.
         for(int i=0;i<arr->nx; ++i)
         {
            for(int j=0;j<arr->ny; ++j)
            {    
               for(int k=0; k< 7; ++k) jacob[idx_dat++] = 0;
            }
         }         
         return;
      }
      
      //If positive-definite, now actually calculate
      idx_dat = 0;
   
      for(int i=0;i<arr->nx; ++i)
      {
         for(int j=0;j<arr->ny; ++j)
         { 
            idx_mat = i+j*arr->nx;
   
            func::gaussian2D_jacobian<realT>(j_tmp, i,j, p[0], p[1], p[2], p[3], p[4], p[5], p[6]); 
            for(int k=0;k<7;++k) jacob[idx_dat++] = j_tmp[k];
         }
      }
   }
   
   void paramNormalizer(realT * p, int dir)
   {
      //Prepare for fit
      if(dir == 1)
      {
         realT a, b, c;
         func::gaussian2D_rot2gen(a,b,c,p[4], p[5], p[6]);
         //std::cout << p[4] << " " << p[5] << " " << p[6] << "\n";
         p[4] = a;
         p[5] = b;
         p[6] = c;
         //std::cout << p[4] << " " << p[5] << " " << p[6] << "\n";
         return;
      }
      
      //Convert after fit
      if(dir == -1)
      {
         realT sigma_x, sigma_y, theta;
         //std::cout << p[4] << " " << p[5] << " " << p[6] << "\n";
         func::gaussian2D_gen2rot(sigma_x, sigma_y, theta, p[4], p[5], p[6]);

         p[4] = sigma_x;
         p[5] = sigma_y;
         p[6] = theta;
         //std::cout << p[4] << " " << p[5] << " " << p[6] << "\n";
         return;
      }
   }
};

///Alias for the fitGaussian2D type fitting the symmetric gaussian.
/** \ingroup gaussian_peak_fit
  */
template<typename realT>
using fitGaussian2Dsym = mx::math::fit::fitGaussian2D<mx::math::fit::gaussian2D_sym_fitter<realT>>;

///Alias for the fitGaussian2D type fitting the general elliptical gaussian.
/** \ingroup gaussian_peak_fit
  */
template<typename realT>
using fitGaussian2Dgen = mx::math::fit::fitGaussian2D<mx::math::fit::gaussian2D_gen_fitter<realT>>;


} //namespace fit
} //namespace math

} //namespace mx

#endif //fitGaussian_hpp
