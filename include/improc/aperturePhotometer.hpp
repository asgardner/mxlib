/** \file aperturePhotometer.hpp
  * \brief Class for conducting aperture photometry on an image.
  * \ingroup image_processing_files
  * \author Jared R. Males (jaredmales@gmail.com)
  *
  */

#ifndef improc_aperturePhotometer_hpp
#define improc_aperturePhotometer_hpp

#include <map>
#include <mx/improc/eigenImage.hpp>
#include <mx/improc/imageMasks.hpp>


namespace mx 
{
namespace improc 
{
   
/// Class for performing aperture photometry on images.
/** Designed to very efficiently calculate the cumulative flux as a function of radius,
  * with minimal cost for performing the measurement on multiple images.
  * 
  * This initial version assumes the source is at 0.5*(rows-1) and 0.5*(cols-1).  
  * 
  * First call resize with the desired size.  This organizes the radius vector and 
  * the index image which maps pixels to position in the radius vector.
  * 
  * The call cumPhot with the image, which will sum the flux contained within each radius.
  * 
  * \tparam realT the real floating point type of the data
  */ 
template<typename realT>
class aperturePhotometer
{

protected:
   
   ///Holds the ordered unique radii of the pixels in the image.
   std::vector<realT> m_radius;

   ///Maps a pixel in the image to the position in the radius vector.
   eigenImage<size_t> m_indexIm;

public:
   
   ///Resize the photometer, recalculating the radius vector and index image.
   /** If size is unchanged, nothing is done.
     *
     * \note Source must be at geometric center of image.
     * 
     * \returns 0 on success
     * \returns -1 on error 
     */
   int resize( int sizeX, ///< [in] The new size in rows
               int sizeY  ///< [in] The new size in columns
             )
   {
      //Don't bother if we don't have too.
      if( sizeX == m_indexIm.rows() && sizeY == m_indexIm.cols()) return 0;

      m_indexIm.resize(sizeX, sizeY);
      
      //Get radius of each pixel up front, since we need it twice.
      eigenImage<realT> radIm;
      radIm.resize(sizeX, sizeY);
      radiusImage( radIm ); //mx function to fill in the radius array
      
      std::map<realT, size_t> unrads; // Map of unique radii to their indices in the radius vector.
   
      //First populate the map
      for(int i=0;i<radIm.rows();++i)
      {
         for(int j=0;j<radIm.cols();++j)
         {
            unrads[radIm(i,j)] = 0;
         }
      }
      
      //Now fill in the radius vector, and set map values to the indices
      m_radius.resize(unrads.size());
      typename std::map<realT, size_t>::iterator it;
      int n = 0;
      for(it = unrads.begin(); it!=unrads.end(); ++it)
      {
         m_radius[n] = it->first;
         it->second = n;
         ++n;
      }
      
      //Finally, fill in the index image with index of the radius vector for that pixel.
      for(int i=0;i<radIm.rows();++i)
      {
         for(int j=0;j<radIm.cols();++j)
         {
            it = unrads.find(radIm(i,j));
            m_indexIm(i,j) = it->second;
         }
      }
      
      return 0;
   }

   /// Get the cumulative photometry of an image as a function of radius.
   /**
     * \note Source must be at geometric center of image.
     * \returns 0 on success
     * \returns -1 on error
     */
   int cumPhot( std::vector<realT> & cumPhot, ///< [out] the cumulative photometry at each point in the radius vector.  Resized.
                eigenImage<realT> & im,       ///< [in] the image on which to perform the calculation
                realT maxr = 0                ///< [in] [optional] the maximum radius to which to calculate.  If <= 0 then max possible is used.
              )
   {
      realT xcen = 0.5*(m_indexIm.rows()-1);
      realT ycen = 0.5*(m_indexIm.cols()-1);
      
      if(maxr <= 0) maxr = m_radius.back();
      
      cumPhot.resize(m_radius.size(),0);
    
      //Make sure we stay in bounds
      int x0 = xcen-maxr;
      if(x0 < 0) x0 = 0;
      int x1 = xcen + maxr;
      if(x1 > m_indexIm.rows()-1) x1 = m_indexIm.rows()-1;
      
      int y0 = ycen-maxr;
      if(y0 < 0) y0 = 0;
      int y1 = ycen + maxr;
      if(y1 > m_indexIm.cols()-1) y1 = m_indexIm.rows()-1;
      
      //First get sum at each unique radius
      for(int i= x0; i<= x1; ++i)
      {
         for(int j=y0; j<=y1; ++j)
         {
            if(m_radius[m_indexIm(i,j)] <= maxr) cumPhot[ m_indexIm(i,j) ] += im(i,j);
         }
      }

      //Now perform cumulative sum
      for(int i=1; i<cumPhot.size(); ++i)
      {
         cumPhot[i] += cumPhot[i-1];
      }      
   }

   ///Get the radius value at an index of the vector.
   realT radius( size_t i /**< [in] the index of the radius vector */ )
   {
      return m_radius[i];
   }
   
};

}//namespace improc 
}//namespace mx

#endif //improc_aperturePhotometer_hpp
