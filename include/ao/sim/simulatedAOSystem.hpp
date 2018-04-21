/** \file 
  * \author Jared R. Males (jaredmales@gmail.com)
  * \brief 
  * \ingroup mxAO_sim_files
  * 
  */

#ifndef simulatedAOSystem_hpp
#define simulatedAOSystem_hpp

#include <iostream>
#include <fstream>

#include "../../improc/imagePads.hpp"
#include "../../wfp/imagingUtils.hpp"
#include "../../wfp/fraunhoferPropagator.hpp"
#include "../../improc/ds9Interface.hpp"

#include "../../improc/fitsFile.hpp"
#include "../../improc/fitsUtils.hpp"

#include "../../improc/eigenImage.hpp"
#include "../../improc/eigenCube.hpp"

#include "../../timeUtils.hpp"
#include "../../sigproc/signalWindows.hpp"

#include "wavefront.hpp"
#include "../aoPaths.hpp"

#include <mx/improc/ds9Interface.hpp>

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

#if 1
template<typename complexWavefrontT, typename realPupilT>
void idealCoronagraph(complexWavefrontT & wf, realPupilT & pupil)
{
   typedef typename complexWavefrontT::Scalar complexT;
   typedef typename realPupilT::Scalar        realT;
      
   Eigen::Map<Eigen::Array<complexT,-1,-1> > eigWf(wf.data(), wf.cols(), wf.rows());
   Eigen::Map<Eigen::Array<realT,-1,-1> > eigPup(pupil.data(), pupil.cols(), pupil.rows());
      
   
   eigWf = eigWf - ((eigWf*eigPup).sum()/(eigPup*eigPup).sum())*eigPup;
   
   
}
#endif
   

      
      
///A simulated AO system.
/**
  *
  * Minimum requirement for _turbSeqT:
  \code
  {
     int wfPS(realT ps); //Sets the wavefront platescale (only needed for cascaded systems).
     int frames(); //Returns the number of frames, sets the number of iterations to run.
     int nextWF(wavefront<realT> & wf); //Fills in the wavefront with phase and amplitude
  };
  \endcode 
  * 
  */
template<typename _realT, typename _wfsT, typename _reconT, typename _filterT, typename _dmT, typename _turbSeqT, typename _coronT>
class simulatedAOSystem
{
public:
   
   bool ds9off;
   
   typedef _realT realT; ///< The floating point type used for all calculations
   
   typedef mx::AO::sim::wavefront<realT> wavefrontT; ///< The wavefront type
   
   typedef Eigen::Array<realT, Eigen::Dynamic, Eigen::Dynamic> imageT; ///<The real image type
   
   typedef mx::imagingArray<std::complex<realT>, mx::fftwAllocator<std::complex<realT> >, 0> complexImageT; ///<The complex image type
   
   typedef _wfsT wfsT;
   typedef _reconT reconT;
   typedef _filterT filterT;
   typedef _dmT dmT;
   typedef _turbSeqT turbSeqT;
   typedef _coronT coronT;
   
   std::string _sysName; ///< The system name for use in mx::AO::path 
   std::string _wfsName; ///< The WFS name for use in the mx::AO::path
   std::string _pupilName; ///< The pupil name for use in the mx::AO::path
   
   bool m_sfImagePlane {false};
   
   wfsT wfs;
   reconT recon;
   filterT filter;
   dmT dm;
   turbSeqT turbSeq;
   
   realT _simStep;
   
   int _wfSz;
   realT _wfPS;
   
   void wfPS(realT wps)
   {
   }
   
   realT _D;
   
   realT _wfsLambda;
   realT _sciLambda;
   
   long _frameCounter;

   bool _loopClosed;
   int _loopClosedDelay;
   int _lowOrdersDelay;
   
   std::string _rmsFile;
   std::ofstream _rmsOut;
   bool _rmsUnwrap;
      
   std::string _ampFile;
   std::ofstream _ampOut;
   
   
   /** Wavefront Outputs
     * @{
     */
   
   bool m_writeWavefronts {true};
   std::string m_wfFileBase {"simAOWF"};
   mx::improc::eigenCube<realT> m_wfPhase;
   mx::improc::eigenCube<realT> m_wfAmp;
   
   int m_nWFPerFile {500};
   int m_currWF {0};
   int m_currWFFile {0};
   
   ///@}
   
   /** Image outputs
     * @{ 
     */

   int m_saveSz {0};
   bool _writeIndFrames;
   
   long _saveFrameStart;
   
   std::string _psfFileBase;
   mx::improc::eigenCube<realT> _psfs;
   imageT _psfOut;
   
   bool _doCoron;
   mx::improc::eigenCube<realT> _corons;
   imageT _coronOut;
   
   int _nPerFile;
   int _currImage;
   
   int _currFile;
   
   complexImageT _complexPupil;
   complexImageT _complexPupilCoron;
   //complexImageT _complexFocal;
   imageT _realFocal;

   imageT _realPupil;
   imageT _realFocalCoron;
   
   coronT m_coronagraph;
   
   //wfp::fraunhoferPropagator<complexImageT> _fi;
         
   
   std::vector<typename filterT::commandT> _delayedCommands;
   int _commandDelay;
   std::vector<int> _goodCommands;
   
   improc::ds9Interface ds9i_coron;
   int display_coron {0};
   int display_coron_counter {0};
   
   improc::ds9Interface ds9i_coron_avg;
   int display_coron_avg {0};
   int display_coron_avg_counter {0};
   
   /** @}
     */
   
   ///Default c'tor
   simulatedAOSystem();
   
   ///Destructor
   ~simulatedAOSystem();
   
   
   ///Initialize the system
   /**
     * Initializes the basic parts of the system.
     * 
     * \returns 0 on success
     * \returns -1 on an error (simulation should not continue if this happens).
     * 
     */ 
   int initSystem( const std::string & sysName,       ///< [in] System name.
                   typename dmT::specT & dmSpec,      ///< [in] DM Specification.
                   const std::string & wfsName,       ///< [in] WFS Name.
                   const std::string & pupilName,     ///< [in] Name of the system pupil.
                   const int & wfSz                   ///< [in] Size of the wavefront used for propagation.
                 );
   
   void initSim( typename reconT::specT & reconSpec,
                 realT simStep,
                 int commandDelay,
                 const std::string & coronName
               );

   imageT _pupil; ///< The system pupil.  This is generally a binary mask and will be applied at various points in the propagation.
   imageT _pupilMask; ///< A pupil mask which is applied once at the beginning of propagation.  Could be apodized and/or different from _pupil.
   
   imageT _postMask;
   imageT _coronPhase;
   
   
   int _npix;
   
   ///Measure the system response matrix
   /** System should be initialized with initSystemCal.
     *
     * \param amp 
     * \param rmatName
     * \param nmodes
     */ 
   void takeResponseMatrix(realT amp, std::string rmatName, int nmodes=0);

   int frames();
   
   void calcOpenLoopAmps(wavefrontT & wf);
   
   void nextWF(wavefrontT & wf);

   void runTurbulence();
   
   
};

template<typename realT, typename wfsT, typename reconT, typename filterT, typename dmT, typename turbSeqT, typename coronT>
simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::simulatedAOSystem()
{
   _frameCounter = 0;
   _loopClosed = false;      
   _loopClosedDelay = 0;
   _lowOrdersDelay = 0;
   
   _rmsUnwrap = false;

   _writeIndFrames = false;
   _saveFrameStart = 0;
   _doCoron = false;
   _nPerFile = 100;
   _currImage = 0;
   _currFile = 0;
   
   m_coronagraph._fileDir = getEnv("MX_AO_DATADIR") + "/" + "coron/"; 
   
   _npix = 0;
   
   ds9i_coron.title("Coronagraph");
   ds9i_coron_avg.title("Coron_Avg");
}
   
template<typename realT, typename wfsT, typename reconT, typename filterT, typename dmT, typename turbSeqT, typename coronT>
simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::~simulatedAOSystem()   
{
   if(_ampOut.is_open()) _ampOut.close();
   
   if(_rmsOut.is_open()) _rmsOut.close();
#if 1
   //Write out the psf and coron cubes one last time
   if(_psfFileBase !="" && _currImage > 0 && _writeIndFrames)
   {
      mx::improc::fitsFile<realT> ff;
         
      std::string fn = _psfFileBase + "_psf_" + mx::convertToString(_currFile) + ".fits";
   
      BREAD_CRUMB;
         
      ff.write(fn, _psfs);//_psfs.data(), _psfs.rows(), _psfs.cols(), _currImage);
      
      if(_doCoron)
      {
         std::string fn = _psfFileBase + "_coron_" + mx::convertToString(_currFile) + ".fits";
            
         BREAD_CRUMB;
         
         ff.write(fn, _corons);// _corons.data(), _corons.rows(), _corons.cols(), _currImage);
      }
   }
#endif

}

template<typename realT, typename wfsT, typename reconT, typename filterT, typename dmT, typename turbSeqT, typename coronT>
int simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::initSystem( const std::string & sysName,
                                                                                        typename dmT::specT & dmSpec,
                                                                                        const std::string & wfsName,
                                                                                        const std::string & pupilName,
                                                                                        const int & wfSz )
{
   _sysName = sysName;
   _wfsName = wfsName;
   _pupilName = pupilName;
      
   improc::fitsFile<realT> ff;
   improc::fitsHeader head;

   //Initialize the pupil.
         
   std::string pupilFile = mx::AO::path::pupil::pupilFile(_pupilName);
         
   ff.read(_pupil, head, pupilFile);
   
   _D = head["PUPILD"].Value<realT>(); //pupilD;
   _wfPS = head["SCALE"].Value<realT>();
   
   //Set the wavefront size.
   _wfSz = wfSz;
   wfs.wfSz(_wfSz);
   
   //Turbulence sequence
   turbSeq._pupil = &_pupil;
   turbSeq.wfPS(_wfPS);

   //DM Initialization.
   dm.initialize( dmSpec, _pupilName);
   
   wfs.linkSystem(*this);
   
   _loopClosed = false;
      
   return 0;
   
}

template<typename realT, typename wfsT, typename reconT, typename filterT, typename dmT, typename turbSeqT, typename coronT>
void simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::initSim( typename reconT::specT & reconSpec,
                                                                                      realT simStep,
                                                                                      int commandDelay,
                                                                                      const std::string & coronName
                                                                                    )
{
   
   improc::fitsFile<realT> ff;
   improc::fitsHeader head;

   _simStep = simStep;
   wfs.simStep(_simStep);

   
   filter.initialize(dm.nModes());
   
   recon.initialize(*this, reconSpec);
   dm.calAmp(recon.calAmp());
   
   _loopClosed = false;
   
   _commandDelay = commandDelay;
   
   _delayedCommands.resize((_commandDelay+1)*5);
   _goodCommands.resize((_commandDelay+1)*5);
   
   m_coronagraph.wfSz(_wfSz);
   m_coronagraph.loadCoronagraph(coronName);
}


// template<typename realT, typename wfsT, typename reconT, typename filterT, typename dmT, typename turbSeqT, typename coronT>
// void simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::initSystemCal( const std::string & sysName,
//                                                                                            const std::string & dmName,
//                                                                                            const std::string & wfsName,
//                                                                                            const std::string & pupilName,
//                                                                                            const std::string & basisName,
//                                                                                            const bool & basisOrtho,
//                                                                                            const int & wfSz )
// {
//    _sysName = sysName;
//    _dmName = dmName;
//    _wfsName = wfsName;
//    _pupilName = pupilName;
//    _basisName = basisName;
//    _basisOrtho = basisOrtho;
//    
//    mx::fitsFile<realT> ff;
//    mx::fitsHeader head;
// 
//          
//    std::string pupilFile = mx::AO::path::pupil::pupilFile(_pupilName);
//          
//    ff.read(pupilFile, _pupil, head);
//    
//    //_D = head["SCALE"].Value<realT>(); //pupilD;
//    _D = head["PUPILD"].Value<realT>(); //pupilD;
//    _wfPS = head["SCALE"].Value<realT>();
//    
//    _wfSz = wfSz;
//    wfs.wfSz(_wfSz);
//    
//    _wfPS = _D/std::max(_pupil.rows(), _pupil.cols());
//    turbSeq.wfPS(_wfPS);
// 
//    //DM Initialization.
//    dm.initialize( _dmName, _basisName, _basisOrtho, _pupilName);
//    
//    //dm.loadModes(basisSet, pupil);
// 
//    wfs.linkSystem(*this);
//    
//    _loopClosed = false;
//    
//    
// }

template<typename realT, typename wfsT, typename reconT, typename filterT, typename dmT, typename turbSeqT, typename coronT>
void simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::takeResponseMatrix( realT amp, 
                                                                                                std::string rmatID, 
                                                                                                int nmodes )
{
   complexImageT cpup;
   cpup.resize(_wfSz, _wfSz);
   
   recon.initializeRMat(dm.nModes(), amp, wfs.detRows(), wfs.detCols());
      
   typename filterT::commandT measureVec;
   
   wavefrontT currWF;
   currWF.setAmplitude(_pupil);
    
   wfs.iTime(1);
   wfs.roTime(1);
   wfs.detector.noNoise(true);

   double tO,tF, t0, t1, t_applyMode = 0, t_senseWF = 0, t_calcMeas = 0, t_accum = 0;

   std::cerr << dm.nModes() << "\n";
   
   tO = get_curr_time();
   
   improc::ds9Interface ds9;
      
   for(int i=0;i< dm.nModes(); ++i)
   {
      BREAD_CRUMB;
      
      currWF.setPhase(_pupil*0);
      
      realT s_amp = amp;
      if(nmodes>0 && i>= nmodes) 
      {
         s_amp =0;
         wfs.detectorImage.image.setZero();
      }
      else
      {
         t0 = get_curr_time();
         dm.applyMode(currWF, i, s_amp, 0.8e-6);
         t1 = get_curr_time();
         t_applyMode += t1-t0;
      
         BREAD_CRUMB;
         
         t0 = get_curr_time();
         wfs.senseWavefrontCal(currWF);
         t1 = get_curr_time();
         
         t_senseWF += t1-t0;
      }
      
      BREAD_CRUMB;
      
      t0 = get_curr_time();
      
      recon.calcMeasurement(measureVec, wfs.detectorImage);
      
      //ds9(wfs.detectorImage.image);
      
      t1 = get_curr_time();
 
      t_calcMeas += t1-t0;
      
      BREAD_CRUMB;

      t0 = get_curr_time();
      recon.accumulateRMat(i, measureVec, wfs.detectorImage);
      t1 = get_curr_time();
      t_accum += t1-t0;
      
      BREAD_CRUMB;
      
      //ds9_display(1, wfs.detectorImage.data(), wfs.detectorImage.cols(), wfs.detectorImage.rows(),1, mx::getFitsBITPIX<realT>());
   }
   tF = get_curr_time();
   std::cout << ( (realT) dm.nModes())/(tF - tO) << " Hz\n";
   
   std::cout << t_applyMode/dm.nModes() << " " << t_senseWF/dm.nModes() << " " << t_calcMeas/dm.nModes() << " " << t_accum/dm.nModes();
   std::cout << " " << dm.t_mm/dm.nModes() << " " << dm.t_sum/dm.nModes() << " " << "\n";
   
   std::string fname;
   fname = mx::AO::path::sys::cal::rMat(_sysName, dm.name(), _wfsName, _pupilName, dm.basisName(), rmatID, true);
   
   std::cout << fname << "\n";
   recon.saveRMat(fname);
   
   fname = mx::AO::path::sys::cal::rImages(_sysName, dm.name(), _wfsName, _pupilName, dm.basisName(), rmatID, true);
   recon.saveRImages(fname);
   
}
   
template<typename realT, typename wfsT, typename reconT, typename filterT, typename dmT, typename turbSeqT, typename coronT>
int simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::frames()
{
   return turbSeq.frames();
}

/*
template<typename realT, typename wfsT, typename reconT, typename filterT, typename dmT, typename turbSeqT, typename coronT>
void simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::calcOpenLoopAmps(wavefrontT & wf)
{
    BREAD_CRUMB;
   
   int npix = _pupil->sum();
   
   imageT olAmps.resize(1, dm.nModes());
   
   #pragma omp parallel for
   for(int j=0; j< dm.nModes; ++j)
   {
      realT amp;
      
      
      amp = (wf.phase*dm._infF->image(j)).sum()/ npix;
      
      olAmps(0,j) = amp;
   }

   
}
*/

template<typename realT, typename wfsT, typename reconT, typename filterT, typename dmT, typename turbSeqT, typename coronT>
void simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::nextWF(wavefrontT & wf)
{
   
   BREAD_CRUMB;
      
   if(_npix == 0)
   {
      _npix = _pupil.sum();
   }
   
   BREAD_CRUMB;
   
   turbSeq.nextWF(wf);
   wf.iterNo = _frameCounter;
   
   //std::cout << "Input Photons: " << wf.amplitude.square().sum() << "\n";
      
   BREAD_CRUMB;
   
   //Mean subtraction on the system pupil.  
   realT mn = (wf.phase * _pupil).sum()/_npix;
   
   
   //Apply the pupil mask just once.
   wf.phase = (wf.phase-mn)*_pupil;


   BREAD_CRUMB;
   
  /* 
   if(_openLoopAmps)
   {
      calcOpenLoopAmps(wf);
   }*/
   
   
   BREAD_CRUMB;
   
   realT rms_ol = sqrt(  wf.phase.square().sum()/ _npix );
   
   BREAD_CRUMB;
   
   typename filterT::commandT measuredAmps, commandAmps;
   
   BREAD_CRUMB;
   
   filter.initMeasurements(measuredAmps, commandAmps);
   
   if(_frameCounter == 0)
   {
      for(int i=0; i<_delayedCommands.size();++i) 
      {
         _goodCommands[i] = 0;
      }
   }
   
   
   BREAD_CRUMB;

//   _wfsLambda = 0.78e-6;;
   
   BREAD_CRUMB;
   if(_loopClosed) dm.applyShape(wf, _wfsLambda);

   BREAD_CRUMB;
   if(_loopClosed)
   {
      bool newCV;
      
      BREAD_CRUMB;
   
      newCV = wfs.senseWavefront(wf);
         
      if(newCV)
      {         
         BREAD_CRUMB;
         
         recon.reconstruct(measuredAmps, wfs.detectorImage);
         
         BREAD_CRUMB;
         
         if(_ampFile != "")
         {
            if(!_ampOut.is_open())
            {
               _ampOut.open(_ampFile);
               _ampOut << std::scientific;
            }
            
            _ampOut << measuredAmps.iterNo << "> ";
            for(int i=0;i<measuredAmps.measurement.cols(); ++i)
            {
               _ampOut << measuredAmps.measurement(0,i) << " ";
            }
            _ampOut << std::endl;
         }

         BREAD_CRUMB;
         
         int nAmps = ( _frameCounter % _delayedCommands.size() );
                  
         _delayedCommands[nAmps].measurement = measuredAmps.measurement;
         _delayedCommands[nAmps].iterNo = measuredAmps.iterNo;
         
         _goodCommands[nAmps] = 1;
         
         
         //std::cerr << "nAmps: " << nAmps <<  " " << 1 << "\n";
      }
      else
      {
         int nAmps = ( _frameCounter % _delayedCommands.size() );
         _goodCommands[nAmps] = 0;
         //std::cerr << "nAmps: " << nAmps <<  " " << 0 << "\n";
      }
      
      
      int _currCommand = ( _frameCounter % _delayedCommands.size() ) - _commandDelay;
      if(_currCommand < 0) _currCommand += _delayedCommands.size();
      
      
      //std::cerr << "\t\t Current Command: " << ( _frameCounter % _delayedCommands.size() ) << " " << _currCommand << " " << _goodCommands[_currCommand] << "\n";
      if(_goodCommands[_currCommand])
      {
         BREAD_CRUMB;
         
         filter.filterCommands(commandAmps, _delayedCommands[_currCommand], _frameCounter);
            
         BREAD_CRUMB;
         
         dm.setShape(commandAmps);
      }
      
         
   }
   else
   {
      int nAmps = ( _frameCounter % _delayedCommands.size() );
      _goodCommands[nAmps] = 0;
      //std::cerr << "nAmps: " << nAmps <<  " " << 0 << "\n";
   }
   
   
   //**** If the _postMask isn't set, set it to _pupil ****//
   if(_postMask.rows() != _pupil.rows())
   {
      _postMask = _pupil;
   }
   
   
    //Mean subtraction on the system pupil.  
   mn = (wf.phase * _pupil).sum()/_npix;   
   wf.phase = (wf.phase-mn)*_pupil;
   
   //**** Calculate RMS phase ****//
   realT rms_cl;   
   rms_cl = sqrt( wf.phase.square().sum()/ _postMask.sum() );
   
   std::cout << _frameCounter << " WFE: " << rms_ol << " " << rms_cl << " [rad rms phase]\n";

   if(m_sfImagePlane)
   {
      wfs._filter.filter(wf.phase);
   }
   
   BREAD_CRUMB;
   
   if(_rmsFile != "")
   {
      if(! _rmsOut.is_open() )
      {
         _rmsOut.open(_rmsFile);
         _rmsOut << "#open-loop-wfe    closed-loop-wfe  [rad rms phase]\n";
      }
      _rmsOut << rms_ol << " " << rms_cl << std::endl;
   }
      
   BREAD_CRUMB;
   
   if( m_writeWavefronts )
   {
      if( m_wfPhase.rows() != wf.phase.rows() || m_wfPhase.cols() != wf.phase.cols() || m_wfPhase.planes() != m_nWFPerFile || 
             m_wfAmp.rows() != wf.amplitude.rows() || m_wfAmp.cols() != wf.amplitude.cols() || m_wfAmp.planes() != m_nWFPerFile )
      {
         m_wfPhase.resize( wf.phase.rows(), wf.phase.cols(), m_nWFPerFile);
         m_wfAmp.resize( wf.amplitude.rows(), wf.amplitude.cols(), m_nWFPerFile);
         
         m_currWF = 0;
      }
      
      m_wfPhase.image(m_currWF) = wf.phase;
      m_wfAmp.image(m_currWF) = wf.amplitude;
      
      ++m_currWF;
      
      if( m_currWF >= m_nWFPerFile )
      {
         std::cerr << "Write to WF file here . . . \n";
         
         mx::improc::fitsFile<realT> ff;
         
         std::string fn = m_wfFileBase + "_phase_" + mx::convertToString<int,5,'0'>(m_currWFFile) + ".fits";
         ff.write(fn, m_wfPhase);
         
         fn = m_wfFileBase + "_amp_" + mx::convertToString<int,5,'0'>(m_currWFFile) + ".fits";
         ff.write(fn, m_wfAmp);
         
         ++m_currWFFile;
         m_currWF = 0;
      }
      
   }
   
   BREAD_CRUMB;
   
   if(_psfFileBase != "" && _frameCounter > _saveFrameStart)
   {
      if(_psfOut.rows() == 0 || (_psfs.planes() == 0 && _writeIndFrames))
      {
         if(m_saveSz <= 0) m_saveSz = _wfSz;
         
         if(_writeIndFrames) _psfs.resize(m_saveSz, m_saveSz, _nPerFile);
         _psfOut.resize(m_saveSz, m_saveSz);
         _psfOut.setZero();
         
         _realFocal.resize(m_saveSz, m_saveSz);
         
         if(_doCoron) 
         {
            if(_writeIndFrames) _corons.resize(m_saveSz, m_saveSz, _nPerFile);
            _coronOut.resize(m_saveSz, m_saveSz);
            _coronOut.setZero();
            
            _realFocalCoron.resize(m_saveSz, m_saveSz);
         }            
      }

      BREAD_CRUMB;

      //Propagate Coronagraph
      if(_doCoron)
      {
         wf.getWavefront(_complexPupilCoron, _wfSz);
   
         m_coronagraph.propagate(_realFocalCoron, _complexPupilCoron);
   
         if(_writeIndFrames) _corons.image(_currImage) = _realFocalCoron;
   
         _coronOut += _realFocalCoron;
      }
      
      //Propagate PSF
      wf.getWavefront(_complexPupil, _wfSz);

      m_coronagraph.propagateNC(_realFocal, _complexPupil);

      if(_writeIndFrames) _psfs.image(_currImage) = _realFocal;

      _psfOut += _realFocal;
      
      
      
      BREAD_CRUMB;
      
      if( _doCoron && display_coron > 0)
      {
         ++display_coron_counter;
      
         if(display_coron_counter >= display_coron)
         {
            realT mxpk = _realFocal.maxCoeff();
            _realFocalCoron /= mxpk; 
            
            ds9i_coron(_realFocalCoron);
            display_coron_counter = 0;
         }
      }

      BREAD_CRUMB;

      if( _doCoron && display_coron_avg > 0)
      {
         ++display_coron_avg_counter;
      
         if(display_coron_avg_counter >= display_coron_avg)
         {
            realT mxpk = _psfOut.maxCoeff();
            _realFocal = _coronOut / mxpk; 
            
            ds9i_coron_avg(_realFocal);
            display_coron_avg_counter = 0;
            
            _psfOut.setZero();
            _coronOut.setZero();
         }
      }
      BREAD_CRUMB;
      
      
      ++_currImage;
#if 1
      if(_currImage >= _nPerFile && _writeIndFrames)
      {
         mx::improc::fitsFile<realT> ff;
         
         std::string fn = _psfFileBase + "_psf_" + mx::convertToString(_currFile) + ".fits";
   
         BREAD_CRUMB;
         
         ff.write(fn, _psfs);// _psfs.data(), _psfs.rows(), _psfs.cols(), _psfs.planes());
      
         if(_doCoron)
         {
            std::string fn = _psfFileBase + "_coron_" + mx::convertToString(_currFile) + ".fits";
            
            BREAD_CRUMB;
            
            ff.write(fn, _corons);//_corons.data(), _corons.rows(), _corons.cols(), _corons.planes());
         }

         ++_currFile;
         _currImage = 0;
      }
#endif    
   }//if(_psfFileBase != "" && _frameCounter > _saveFrameStart)
   
   ++_frameCounter;
   
}//void simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::nextWF(wavefrontT & wf)

template<typename realT, typename wfsT, typename reconT, typename filterT, typename dmT, typename turbSeqT, typename coronT>
void simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::runTurbulence()
{   
   wavefrontT currWF;


   _wfsLambda = wfs.lambda();
   
   for(int i=0;i<turbSeq.frames();++i)
   { 
      //std::cout << i << "/" << turbSeq.frames() << "\n" ;
   
      
      if(i == 0) 
      {
         turbSeq._loopClosed = true;
         //wfs.applyFilter = false;
      }
      
      if(i == _loopClosedDelay) _loopClosed = true;
      
      
   
      BREAD_CRUMB;
      
      nextWF(currWF);
      
      BREAD_CRUMB;
      
   }
   
   if(_psfFileBase != "")
   {
      improc::fitsFile<realT> ff;
      std::string fn = _psfFileBase + "_psf.fits";
      BREAD_CRUMB;
      ff.write(fn, _psfOut);
      
      if(_doCoron)
      {
         BREAD_CRUMB;
         fn = _psfFileBase + "_coron.fits";
         ff.write(fn, _coronOut);
      }
   }
   
   BREAD_CRUMB;
      
}//void simulatedAOSystem<realT, wfsT, reconT, filterT, dmT, turbSeqT, coronT>::runTurbulence()
   
} //namespace sim 
} //namespace AO
} //namespace mx

#endif //simulatedAOSystem_hpp
