/** \file eigenLapack.hpp
  * \brief Interfaces to Lapack and BLAS for Eigen-like arrays.
  * 
  * \author Jared R. Males (jaredmales@gmail.com)
  * 
  * \ingroup gen_math_files
  *
  */

#ifndef eigenLapack_hpp
#define eigenLapack_hpp

#pragma GCC system_header
#include <Eigen/Dense>


#include <cmath>

#include "templateBLAS.hpp"
#include "templateLapack.hpp"

//#include "vectorUtils.hpp"

#include "../gnuPlot.hpp"


namespace mx
{
namespace math
{

/// Calculates the lower triangular part of the covariance matrix of ims.
/** Uses cblas_ssyrk.  cv is resized to ims.cols() X ims.cols().
  * Calculates \f$ cv = A^T*A \f$.
  * 
  *
  * \tparam eigenT1 is the eigen matrix/array type of cv.
  * \tparam eigenT2 is the eigen matrix/array type of ims 
  * 
  * \ingroup eigen_lapack
  */ 
template<typename eigenT1, typename eigenT2>
void eigenSYRK( eigenT1 &cv,  ///< [out] is the eigen matrix/array where to store the result
                const eigenT2 &ims ///< [in] is the eigen matrix/array (images as columns) to calculate the covariance of
              )
{
   cv.resize(ims.cols(), ims.cols());
   
   math::syrk<typename eigenT1::Scalar>(/*const enum CBLAS_ORDER Order*/ CblasColMajor, /*const enum CBLAS_UPLO Uplo*/ CblasLower,
                 /*const enum CBLAS_TRANSPOSE Trans*/ CblasTrans, /*const MXLAPACK_INT N*/ims.cols(), /*const MXLAPACK_INT K*/ ims.rows(),
                 /*const float alpha*/ 1.0, /*const float *A*/ims.data(), /*const MXLAPACK_INT lda*/ ims.rows(),
                 /*const float beta*/ 0., /*float *C*/ cv.data(), /*const MXLAPACK_INT ldc*/ cv.rows());
   
}   



/// A struct to hold the working memory for eigenSYEVR and maintain it between calls if desired.
/** \todo this should have the working memory for the first exploratory call to ?syevr as well.
  */
template<typename floatT>
struct syevrMem
{
   MXLAPACK_INT sizeISuppZ;
   MXLAPACK_INT sizeWork;
   MXLAPACK_INT sizeIWork;
   
   MXLAPACK_INT *iSuppZ;
   
   floatT *work;
   
   MXLAPACK_INT *iWork;
   
   syevrMem()
   {
      sizeISuppZ = 0;
      sizeWork = 0;
      sizeIWork = 0;
      
      iSuppZ = 0;
      work = 0;
      iWork = 0;
   }
   
   ~syevrMem()
   {
      if(iSuppZ) ::free(iSuppZ);
      if(work) ::free(work);
      if(iWork) ::free(iWork);

   }
        
   void free()
   {
      if(iSuppZ) ::free(iSuppZ);
      sizeISuppZ = 0;
      
      if(work) ::free(work);
      sizeWork = 0;
      
      if(iWork) ::free(iWork);
      sizeIWork = 0;
   }
   
};

/// Calculate select eigenvalues and eigenvectors of an Eigen Array
/** Uses the templateLapack wrapper for syevr.
  * 
  * \tparam cvT is the scalar type of X (a.k.a. the covariance matrix)
  * \tparam calcT is the type in which to calculate the eigenvectors/eigenvalues
  *
  * \returns -1000 on an malloc allocation error.
  * \returns the return code from syevr (info) otherwise.
  * 
  * \ingroup eigen_lapack
  */
template<typename cvT, typename calcT>
MXLAPACK_INT eigenSYEVR( Eigen::Array<calcT, Eigen::Dynamic, Eigen::Dynamic> &eigvec, ///< [out] will contain the eigenvectors as columns
                         Eigen::Array<calcT, Eigen::Dynamic, Eigen::Dynamic> &eigval, ///< [out] will contain the eigenvalues
                         Eigen::Array<cvT, Eigen::Dynamic, Eigen::Dynamic> &X,        ///< [in] is a square matrix which is either upper or lower (default) triangular
                         int ev0=0,                                                   ///< [in] [optional] is the first desired eigenvalue (default = 0)
                         int ev1=-1,                                                  ///< [in] [optional] if >= ev0 then this is the last desired eigenvalue.  If -1 all eigenvalues are returned.
                         char UPLO = 'L',                                             ///< [in] [optional] specifies whether X is upper ('U') or lower ('L') triangular.  Default is ('L').
                         syevrMem<calcT> * mem = 0                                    ///< [in] [optional] holds the working memory arrays, can be re-passed to avoid unnecessary re-allocations
                       ) 
{     
   MXLAPACK_INT  numeig, info;
   char RANGE = 'A';
   
   MXLAPACK_INT localMem = 0;
   
   if(mem == 0)
   {
      mem = new syevrMem<calcT>;
      localMem = 1;
   }
      
   MXLAPACK_INT n = X.rows();
   
   MXLAPACK_INT IL = 1;
   MXLAPACK_INT IU = n;
   if(ev0 >= 0 && ev1 >= ev0)
   {
      RANGE = 'I';
      IL = ev0+1; //This is FORTRAN, after all
      IU = ev1;
   }
   
   
   eigvec.resize(n,IU-IL+1);
   eigval.resize(n, 1); 
      
   //Copy X, casting to calcT
   Eigen::Array<calcT, Eigen::Dynamic, Eigen::Dynamic> Xc = X.template cast<calcT>();
   
   if( mem->sizeISuppZ < 2*n)
   {
      if(mem->iSuppZ) free(mem->iSuppZ);
         
      mem->sizeISuppZ = 2*n;
      mem->iSuppZ = (MXLAPACK_INT *) malloc (mem->sizeISuppZ*sizeof(MXLAPACK_INT));
   
      if ( mem->iSuppZ==NULL ) 
      {
         mxError("eigenSYEVR", MXE_ALLOCERR, "malloc failed in eigenSYEVR."); 
         if(localMem) delete mem;
         return -1000;
      }
   }
   
   
   //  Allocate minimum allowed sizes for workspace
   MXLAPACK_INT sizeWork = 26*n;
   calcT * work = (calcT *) malloc (sizeWork*sizeof(calcT));
   
   
   MXLAPACK_INT sizeIWork = 10*n;
   MXLAPACK_INT * iWork = (MXLAPACK_INT *) malloc (sizeIWork*sizeof(MXLAPACK_INT));

   //  Query for optimum sizes for workspace 
   info=math::syevr<calcT>('V', RANGE, UPLO, n, Xc.data(), n, 0, 0, IL, IU, math::lamch<calcT>('S'), &numeig, eigval.data(), eigvec.data(), n, mem->iSuppZ, work, -1, iWork, -1);

   if(info != 0)
   {
      mxError("eigenSYEVR", MXE_LAPACKERR, "error from SYEVR");
      if(localMem) delete mem;
      free(work);
      return info;
   }
   
   // Now allocate optimum sizes
   /* -- tested increasing by x10, didn't improve performance at all 
    */
   if( mem->sizeWork <  ((MXLAPACK_INT) work[0])*(1))
   {
      if(mem->work) free(mem->work);
  
      mem->sizeWork = ((MXLAPACK_INT) work[0])*1;
      mem->work = (calcT *) malloc ((mem->sizeWork)*sizeof(calcT));
   }
   free(work);
   
   
   if(mem->sizeIWork < iWork[0]*1)
   {
      if(mem->iWork) free(mem->iWork);

      mem->sizeIWork = iWork[0]*1; 
      mem->iWork = (MXLAPACK_INT *) malloc ((mem->sizeIWork)*sizeof(MXLAPACK_INT));
   }   
   free(iWork);
   

   if ((mem->work==NULL)||(mem->iWork==NULL)) 
   {
      mxError("eigenSYEVR", MXE_ALLOCERR, "malloc failed in eigenSYEVR.");
      if(localMem) delete mem;
      return -1000;
   }
                
   // Now actually do the calculationg
   info=math::syevr<calcT>('V', RANGE, UPLO, n, Xc.data(), n, 0, 0, IL, IU, math::lamch<calcT>('S'), &numeig, eigval.data(), eigvec.data(), n, mem->iSuppZ, mem->work, mem->sizeWork, mem->iWork, mem->sizeIWork);     
   
    /*  Cleanup and exit  */
      
   if(localMem) delete mem;
   
   return info;
}       

///Calculate the K-L modes, or principle components, given a covariance matrix.
/** Eigen-decomposition of the covariance matrix is performed using \ref eigenSYEVR().
  * 
  * \tparam evCalcT is the type in which to perform eigen-decomposition.
  * \tparam eigenT is a 2D Eigen-like type
  * \tparam eigenT1 is a 2D Eigen-like type.  
  * 
  * \ingroup eigen_lapack
  */
template<typename _evCalcT = double, typename eigenT, typename eigenT1>
MXLAPACK_INT calcKLModes( eigenT & klModes, ///< [out] on exit contains the K-L modes (or P.C.s)
                          eigenT & cv, ///< [in] a lower-triangle (in the Lapack sense) square covariance matrix.
                          const eigenT1 & Rims, ///< [in] The reference data.  cv.rows() == Rims.cols().
                          int n_modes = 0, ///< [in] [optional] Tbe maximum number of modes to solve for.  If 0 all modes are solved for.
                          syevrMem<_evCalcT> * mem = 0 ///< [in] [optional] A memory structure which can be re-used by SYEVR for efficiency.
                        )
{
   typedef _evCalcT evCalcT;
   typedef typename eigenT::Scalar realT;
   
   eigenT evecs, evals;
   
   Eigen::Array<evCalcT, Eigen::Dynamic, Eigen::Dynamic> evecsd, evalsd;
   
   if(cv.rows() != cv.cols())
   {
      std::cerr << "Non-square covariance matrix input to calcKLModes\n";
      return -1;
   }

   if(cv.rows() != Rims.cols())
   {
      std::cerr << "Covariance matrix - reference image size mismatch in klip_klModes\n";
      return -1;
   }


   MXLAPACK_INT tNims = cv.rows();
   MXLAPACK_INT tNpix = Rims.rows();

   if(n_modes <= 0 || n_modes > tNims) n_modes = tNims;

   //Calculate eigenvectors and eigenvalues
   /* SYEVR sorts eigenvalues in ascending order, so we specifiy the top n_modes
    */   
   MXLAPACK_INT info = eigenSYEVR<realT, evCalcT>(evecsd, evalsd, cv, tNims - n_modes, tNims, 'L', mem);
      
   if(info !=0 ) 
   {
      std::cerr << "info =" << info << "\n";
      exit(0);
   }
   
   evecs = evecsd.template cast<realT>();
   evals = evalsd.template cast<realT>();
      
   //Normalize the eigenvectors
   for(MXLAPACK_INT i=0;i< n_modes; ++i)
   {
      evecs.col(i) = evecs.col(i)/sqrt(evals(i));
   }

   klModes.resize(n_modes, tNpix);

   
   //Now calculate KL images
   /*
    *  KL = E^T * R  ==> C = A^T * B
    */
   gemm<realT>(CblasColMajor, CblasTrans, CblasTrans, n_modes, tNpix,
                              tNims, 1., evecs.data(), cv.rows(), Rims.data(), Rims.rows(),
                                 0., klModes.data(), klModes.rows());

   
} //calcKLModes        

///Compute the SVD of an Eigen::Array using LAPACK's xgesdd
/** Computes the SVD of A, \f$ A = U S V^T \f$.
  * 
  * \param[out] U the A.rows() x A.rows() left matrix
  * \param[out] S the A.cols() x 1 matrix of singular values
  * \param[out] VT the A.cols() x A.cols() right matrix, note this is the transpose.
  * \param[in] A the input matrix to be decomposed
  *
  * \returns 
  * \parblock
  *     0 on success
  *     -i on error in ith parameter (from LAPACK xgesdd)
  *     >0 did not converge (from LAPACK xgesdd)
  * \endparblock
  * 
  * \tparam dataT is either float or double.
  * 
  * \ingroup eigen_lapack
  */ 
template<typename dataT>
MXLAPACK_INT eigenGESDD( Eigen::Array<dataT,-1,-1> & U, Eigen::Array<dataT,-1,-1> & S, Eigen::Array<dataT,-1,-1> & VT, Eigen::Array<dataT,-1,-1> & A )
{
   char JOBZ = 'A';
   MXLAPACK_INT M = A.rows();
   MXLAPACK_INT N = A.cols();
   MXLAPACK_INT LDA = M;
   S.resize(N,1);
   U.resize(M,M);
   MXLAPACK_INT LDU = M;
   VT.resize(N,N);
   MXLAPACK_INT LDVT = N;
   
   dataT wkOpt;
   MXLAPACK_INT LWORK = -1;
   
   MXLAPACK_INT * IWORK = new MXLAPACK_INT[8*M];
   MXLAPACK_INT INFO;
   
   math::gesdd<dataT>(JOBZ, M, N, A.data(), LDA, S.data(), U.data(), LDU, VT.data(), LDVT, &wkOpt, LWORK, IWORK, INFO);
   
   LWORK = wkOpt;
   //delete WORK;
   dataT *WORK = new dataT[LWORK];
   
   INFO = math::gesdd<dataT>(JOBZ, M, N, A.data(), LDA, S.data(), U.data(), LDU, VT.data(), LDVT, WORK, LWORK, IWORK, INFO);
   
   delete[] WORK;
   delete[] IWORK;
   
   return INFO;
}

#define MX_PINV_NO_INTERACT 0
#define MX_PINV_PLOT 1
#define MX_PINV_ASK 2
#define MX_PINV_ASK_NMODES 4

///Calculate the pseudo-inverse of a patrix using the SVD
/** First computes the SVD of A, \f$ A = U S V^T \f$, using eigenGESDD.  Then the psuedo-inverse is calculated as \f$ A^+ = V S^+ U^T\f$.
  * 
  * \param[out] PInv the pseudo-inverse of A
  * \param[out] condition The final condition number.
  * \param[out] nRejected the number of eigenvectors rejected
  * \param[out] U
  * \param[out] S
  * \param[out] VT
  * \param[in] A the matrix to invert
  * \param[in] maxCondition he maximum condition number desired, which is used to threshold the singular values.  Set to 0 use include all eigenvalues/vectors.  This is ignored if interactive.
  * \param[in] interact [optional] a bitmask controlling interaction.  If (interact & MX_PINV_PLOT) is true, then gnuPlot is used to display the singular values.  If (interact & MX_PINV_ASK) is true
  *                                 then the minimum singular value threshold is requested from the user using stdin. 
  * \tparam dataT is either float or double.
  * 
  * \ingroup eigen_lapack
  */
template<typename dataT>
int eigenPseudoInverse(Eigen::Array<dataT, -1, -1> & PInv,
                       dataT & condition,
                       int & nRejected, 
                       Eigen::Array<dataT, -1, -1> & U,
                       Eigen::Array<dataT, -1, -1> & S,
                       Eigen::Array<dataT, -1, -1> & VT,
                       Eigen::Array<dataT, -1, -1> & A, 
                       dataT & maxCondition,
                       int interact = MX_PINV_NO_INTERACT   )
{
   //Eigen::Array<dataT,-1,-1> S, U, VT;
   
   MXLAPACK_INT info;
   info = eigenGESDD(U,S,VT,A);
   
   if(info != 0) return info;
   
   
   dataT Smax=S.maxCoeff();
   
   
   if(interact & MX_PINV_PLOT)
   {
      gnuPlot gp;
      gp.command("set title \"SVD Singular Values\"");
      gp.logy();
      gp.plot( S.data(), S.rows(), " w lp", "singular values");
   }
   
   if(interact & MX_PINV_ASK && ! (interact & MX_PINV_ASK_NMODES))
   {
      dataT mine;
      std::cout << "Maximum singular value: " << Smax << "\n";
      std::cout << "Minimum singular value: " << S.minCoeff() << "\n";
      std::cout << "Enter singular value threshold: ";
      std::cin >> mine;
   
      if(mine > 0)
      {
         maxCondition = Smax/mine;
      }
      else maxCondition = 0;
   }

   if( interact & MX_PINV_ASK_NMODES)
   {
      unsigned mine;
      std::cout << "Maximum singular value: " << Smax << "\n";
      std::cout << "Minimum singular value: " << S.minCoeff() << "\n";
      std::cout << "Enter number of modes to keep: ";
      std::cin >> mine;
   
      if(mine > 0)
      {
         maxCondition = Smax/S(mine-1,0);
      }
      else maxCondition = 0;
   }
   
   
   dataT threshold = 0;
   if(maxCondition > 0)
   {
      threshold = Smax/maxCondition;
   }
   
   Eigen::Array<dataT, -1,-1> sigma;
   sigma.resize(S.rows(), S.rows());
   sigma.setZero();
   
   condition = 1;
   nRejected = 0;
   for(MXLAPACK_INT i=0; i< S.rows(); ++i)
   {
      if( S(i) >= threshold )
      {
         sigma(i,i) = 1./S(i);
         if(Smax/S(i) > condition) condition = Smax/S(i);
      }
      else
      {
         sigma(i,i) = 0;
         ++nRejected;
      }
   }

   if(interact & MX_PINV_ASK  || interact & MX_PINV_ASK_NMODES)
   {
      dataT mine;
      std::cout << "Modes Rejected: " << nRejected << "\n";
      std::cout << "Condition Number: " << condition << "\n";
   }
   
   Eigen::Array<dataT, -1,-1> PInvTmp;

   PInvTmp = sigma.matrix() * VT.matrix();
   
   PInv = U.block(0,0, U.rows(), PInvTmp.cols()).matrix()*PInvTmp.matrix();

   return 0;
}



///Calculate the pseudo-inverse of a patrix using the SVD
/** First computes the SVD of A, \f$ A = U S V^T \f$, using eigenGESDD.  Then the psuedo-inverse is calculated as \f$ A^+ = V S^+ U^T\f$.
  * This MXLAPACK_INTerface does not provide access to U, S and VT.
  * 
  * \param PInv [out] the pseudo-inverse of A
  * \param condition [out] The final condition number.
  * \param nRejected [out] the number of eigenvectors rejected
  * \param A [in] the matrix to invert
  * \param maxCondition [in] the maximum condition number desired, whichis used to threshold the singular values.  Set to 0 use include all eigenvalues/vectors.  This is ignored if interactive.
  * \param interact [in] [optional] a bitmask controlling interaction.  If (interact & MX_PINV_PLOT) is true, then gnuPlot is used to display the eigenvalues.  If (interact & MX_PINV_ASK) is true
  *                                 then the minimum eigenvaue threshold is requested from the user using stdin. 
  * \tparam dataT is either float or double.
  * 
  * \ingroup eigen_lapack
  */
template<typename dataT>
int eigenPseudoInverse(Eigen::Array<dataT, -1, -1> & PInv,
                       dataT & condition,
                       int & nRejected, 
                       Eigen::Array<dataT, -1, -1> & A, 
                       dataT & maxCondition,
                       int interact = MX_PINV_NO_INTERACT   )
{
   Eigen::Array<dataT,-1,-1> S, U, VT;
   
   return eigenPseudoInverse(PInv, condition, nRejected, U, S, VT, A, maxCondition, interact);
}







} //namespace math
}//namespace mx

#endif //eigenLapack_hpp
