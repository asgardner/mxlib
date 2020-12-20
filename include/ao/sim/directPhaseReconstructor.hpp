#ifndef directPhaseReconstructor_hpp
#define directPhaseReconstructor_hpp


#include "../../improc/eigenImage.hpp"
#include "../../improc/eigenCube.hpp"
#include "../../improc/fitsFile.hpp"
using namespace mx::improc;

#include "../../sigproc/signalWindows.hpp"

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


struct directPhaseReconstructorSpec
{
   std::string dmName; 
   std::string basisName; 
   
   std::string rMatId;
};

/// Direct Phase Reconstructor
/** Calculates modal amplitudes by direct projection of modes onto the phase screen.
  */ 
template<typename realT> 
class directPhaseReconstructor
{
public:
   
   ///The type of the WFS image
   typedef Eigen::Array<realT, -1, -1> imageT;
   
   ///The type of the response matrix
   typedef Eigen::Array<realT, -1, -1> rmatT;
 
   ///The specificaion type.
   typedef directPhaseReconstructorSpec specT;
      
protected:
   
   /** \name The Pupil
     * @{
     */
   imageT * m_pupil {nullptr};
   int m_nPix {0};
   ///@}
   
   /** \name The Basis
     * @{
     */
   improc::eigenCube<realT> *m_modes {nullptr}; ///< The mirror modes, managed by the DM

   int m_nModes {0}; ///<The number of modes to be reconstructed
   
   int m_detRows {0}; ///<The size of the WFS image, in rows
   int m_detCols {0};///<The size of the WFS image, in columns
   ///@}   
   
   /** \name The Reconstructor
     * @{
     */
   Eigen::Array<realT,-1,-1> m_recon; ///< The reconstructor matrix.
   int m_measurementSize {0}; ///<The number of values in the measurement
   std::vector<size_t> * m_idx {nullptr}; /// The offset coordinates of non-zero pixels in the pupil.  Set by the DM.
   ///@}
   
   realT m_calAmp {1e-6}; ///<The calibration amplitude used for response matrix acquisition
   
   imageT m_rMat; ///<The response matrix
      
   eigenCube<realT> m_rImages;
   
   
public:   
   ///Default c'tor
   directPhaseReconstructor();

   template<typename AOSysT>
   void initialize( AOSysT & AOSys, 
                    specT & spec
                  );
   
   
   ///Get the calibration amplitude used in response matrix acquisition (m_calAmp)
   realT calAmp();
   
   
   ///Set the calibration amplitude used in response matrix acquisition (m_calAmp)
   /**
     * \param ca [in] the new calibration amplitude
     */ 
   void calAmp(realT ca);
   
   ///Get the number of modes (m_nModes)
   int nModes();
   
   ///Get the number of detector rows (m_detRows)
   int detRows();
   
   ///Set the number of detector rows (m_detRows)
   void detRows(int dr);

   ///Get the number of detector columns (m_detCols)   
   int detCols();
   
   ///Set the number of detector columns (m_detCols)
   void detCols(int dc);
   
   ///Load the reconstrutor from the specified FITS file 
   /** 
     * \param fname is the name of the FITS file, including path
     */   
   void loadRecon(const std::string & fname);
   
   ///Return the size of the unbinned measurement
   int measurementSize();

   
   ///Calculate the slope measurement
   /**
     * \param slopes [out] a (m_measurementSize X 2)  array of slopes
     * \param wfsImage [in] the WFS image from which to measure the slopes
     */   
   template<typename measurementT, typename wfsImageT>
   void calcMeasurement(measurementT & slopes, wfsImageT & wfsImage);
         
   ///Reconstruct the wavefront from the input image, producing the modal amplitude vector
   template<typename measurementT, typename wfsImageT>
   void reconstruct(measurementT & commandVect, wfsImageT & wfsImage);

   ///Initialize the response matrix for acquisition
   /** 
     * \param nmodes the number of modes 
     * \param calamp the calibration amplitude
     * \param detrows the number of detector rows
     * \param detcols the number of detector columns
     */ 
   void initializeRMat(int nmodes, realT calamp, int detrows,int detcols);
   
   ///Accumalte the next measurement in the response matrix
   /** 
     * \param i the measurement index
     * \param measureVec is the i-th measurement vector
     */ 
   template<typename measurementT>
   void accumulateRMat(int i, measurementT &measureVec);

   
   template<typename measurementT, typename wfsImageT>
   void accumulateRMat(int i, measurementT &measureVec, wfsImageT & wfsImage);
   
   ///Write the accumulated response matrix to disk
   /**
     * \param fname the name, including path, of the response matrix
     */ 
   void saveRMat(std::string fname);
   
   void saveRImages(std::string fname);
};


template<typename realT> 
directPhaseReconstructor<realT>::directPhaseReconstructor()
{
}

template<typename realT> 
template<typename AOSysT>
void directPhaseReconstructor<realT>::initialize( AOSysT & AOSys, 
                                                  specT & spec
                                                )
{
   static_cast<void>(spec);


   m_pupil = &AOSys._pupil;

   m_nPix = m_pupil->sum();
   
   m_modes = &AOSys.dm.m_infF;

   m_nModes = m_modes->planes();
   m_detRows = m_modes->rows();
   m_detCols = m_modes->cols();

   m_idx = &AOSys.dm.m_idx;
         
   m_measurementSize = m_idx->size();
   
   m_recon.resize(m_measurementSize, m_nModes);

   for(int pp=0; pp<m_nModes; ++pp)
   {
      for(int nn=0; nn < m_idx->size(); ++nn)
      {
         m_recon(nn,pp) = *(m_modes->image(pp).data() + (*m_idx)[nn])/m_nPix;
      }
   }
   
   

   
}
 


template<typename realT> 
realT directPhaseReconstructor<realT>::calAmp()
{
   return 0.5*800.0e-9/two_pi<realT>();
}

template<typename realT> 
void directPhaseReconstructor<realT>::calAmp(realT ca)
{
   return;
}

template<typename realT> 
int directPhaseReconstructor<realT>::nModes()
{
   return m_nModes;
}

template<typename realT> 
int directPhaseReconstructor<realT>::detRows()
{
   return m_detRows;
}

template<typename realT> 
int directPhaseReconstructor<realT>::detCols()
{
   return m_detCols;
}

template<typename realT> 
void directPhaseReconstructor<realT>::loadRecon(const std::string & fname)
{
#if 0
   fitsFile<realT> ff;
   fitsHeader head;
   
   ff.read(m_recon, head, fname);
#endif

}

template<typename realT> 
int directPhaseReconstructor<realT>::measurementSize()
{
   return m_measurementSize;
}

template<typename realT>
template<typename measurementT, typename wfsImageT>
void directPhaseReconstructor<realT>::calcMeasurement(measurementT & slopes, wfsImageT & wfsImage)
{
   slopes.measurement.resize(m_measurementSize);
   
   realT * imp = wfsImage.image.data();
   for(int nn=0; nn < m_idx->size(); ++nn)
   {
      slopes.measurement[nn] = *(imp + (*m_idx)[nn]);
   }
   
}
     
template<typename realT> 
template<typename measurementT, typename wfsImageT>
void directPhaseReconstructor<realT>::reconstruct(measurementT & commandVect, wfsImageT & wfsImage)
{
   
#if 0
   measurementT slopes;
   
   calcMeasurement(slopes, wfsImage);
   
   //std::cerr << slopes.measurement.rows() << " " << slopes.measurement.cols() << " " << m_recon.rows() << " " << m_recon.cols() << "\n";
   Eigen::Map<Eigen::Array<realT,-1,-1>> slopes_measurement(slopes.measurement.data(), 1, slopes.measurement.size());
   
   commandVect.measurement.resize(m_nModes);
   //Eigen::Map<Eigen::Array<realT,-1,-1>> commandVect_measurement(commandVect.measurement.data(), 1, commandVect.measurement.size());
   Eigen::Array<realT,-1,-1> commandVect_measurement;
   std::cerr << "ha ha " << std::endl;
   std::cerr << "recon: " << m_recon.rows() << " " << m_recon.cols() << std::endl;
   std::cerr << "slopes: " << slopes_measurement.rows() << " " << slopes_measurement.cols() << std::endl;
   std::cerr << slopes_measurement(0,0) << "\n";
   
   commandVect_measurement = slopes_measurement.matrix()*m_recon.matrix();
   
   
   for(size_t n=0;n<m_nModes;++n) commandVect.measurement[n] = commandVect_measurement(0,n)/m_nPix;
   
   std::cerr << "hee hee " << commandVect.measurement[0] <<  std::endl;
   std::cerr << commandVect_measurement.rows() << " " << commandVect_measurement.cols() << std::endl;
   commandVect.iterNo = wfsImage.iterNo;
   
#endif
#if 1
   BREAD_CRUMB;   
   
   /* Only needed if using "slopes" below:
   measurementT slopes;
   calcMeasurement(slopes, wfsImage);
   */
   
   #pragma omp parallel for
   for(int j=0; j< m_nModes; ++j)
   {
      //The brutest forcest way, slower:
      //commandVect.measurement[j] = (wfsImage.image*m_modes->image(j)).sum()/ m_nPix;
            
      //The fastest non-GPU way:
      realT amp = 0;
      for(size_t k=0; k < m_idx->size(); ++k)
      {
         amp += *(wfsImage.image.data() + (*m_idx)[k])  *m_recon(k,j);
      }
      commandVect.measurement[j] = amp;
      
      /* Slightly slower, using the slopes calc:
      realT amp = 0;
      for(size_t k=0; k < slopes.measurement.size(); ++k)
      {
         amp += slopes.measurement[k]*m_recon(k,j);
      }
      commandVect.measurement[j] = amp;
      */

      
   }
   
   commandVect.iterNo = wfsImage.iterNo;
   
#endif
}

template<typename realT> 
void directPhaseReconstructor<realT>::initializeRMat(int nModes, realT calamp, int detRows, int detCols)
{
   m_nModes = nModes;
   
   m_detRows = detRows;
   m_detCols = detCols;
   
   m_rMat.resize(measurementSize(), nModes);
   m_rMat.setZero();
   
   m_rImages.resize(m_detRows, m_detCols, m_nModes);
}

template<typename realT> 
template<typename measurementT>
void directPhaseReconstructor<realT>::accumulateRMat(int i, measurementT &measureVec)
{
   int l = 0;
   for(int j=0; j<measureVec.measurement.rows(); ++j)
   {
      for(int k=0; k<measureVec.measurement.cols(); ++k)
      {
         m_rMat(l, i) = measureVec.measurement(j,k);
         ++l;
      }
   }
   
  //m_rMat.col(i) = measureVec.measurement.row(0);
}
  
template<typename realT>   
template<typename measurementT,typename wfsImageT>
void directPhaseReconstructor<realT>::accumulateRMat(int i, measurementT &measureVec, wfsImageT & wfsImage)
{
   accumulateRMat(i, measureVec);
   m_rImages.image(i) = wfsImage.image;   
}

template<typename realT> 
void directPhaseReconstructor<realT>::saveRMat(std::string fname)
{
   fitsFile<realT> ff;
   fitsHeader head;
   
   head.append("DETROWS", m_detRows, "WFS detector rows");
   head.append("DETCOLS", m_detCols, "WFS detector cols");
   head.append("CALAMP", m_calAmp, "DM Calibration amplitude");
   head.append("NMODES", m_nModes, "Number of modes included in the response matrix.");
   
   ff.write(fname, m_rMat, head);
}

template<typename realT> 
void directPhaseReconstructor<realT>::saveRImages(std::string fname)
{
   fitsFile<realT> ff;
   fitsHeader head;
   
   
   head.append("DETROWS", m_detRows, "WFS detector rows");
   head.append("DETCOLS", m_detCols, "WFS detector cols");
   head.append("CALAMP", m_calAmp, "DM Calibration amplitude");
   head.append("NMODES", m_nModes, "Number of modes included in the response matrix.");
   
   //ff.write(fname, m_rImages.data(), m_rImages.rows(), m_rImages.cols(), m_rImages.planes(), &head);
   ff.write(fname, m_rImages, head);
}

} //namespace sim
} //namespace AO
} //namespace mx

#endif //directPhaseReconstructor_hpp



