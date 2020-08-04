#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>
#include "../somier.h"


static int count2=0;

void accel_intr(int coreid, int ncores, int n, double (*A)[n][n][n], double (*F)[n][n][n], double M)
{
   int i, j, k;
   unsigned long int rvl, gvl;
   __epi_1xf64 vF0, vF1, vF2, vA0, vA1, vA2;
   double invM = 1/M;
  
  int chunk=n/ncores;
  int start=coreid*(chunk);
  int end = (coreid==ncores-1) ?  n : start+chunk;

   for (i = start; i<end; i++)
      for (j = 0; j<n; j++)
         for (k = 0; k<n; ) {
            rvl = n-k;
            gvl = __builtin_epi_vsetvl(rvl, __epi_e64, __epi_m1);
            __epi_1xf64 v_invM = __builtin_epi_vfmv_v_f_1xf64(invM, gvl);

            vF0 = __builtin_epi_vload_1xf64( &F[0][i][j][k], gvl );
            vA0 = __builtin_epi_vfmul_1xf64(vF0, v_invM, gvl);
            __builtin_epi_vstore_1xf64(&A[0][i][j][k], vA0, gvl);

            vF1 = __builtin_epi_vload_1xf64( &F[1][i][j][k], gvl );
            vA1 = __builtin_epi_vfmul_1xf64(vF1, v_invM, gvl);
            __builtin_epi_vstore_1xf64(&A[1][i][j][k], vA1, gvl);

            vF2 = __builtin_epi_vload_1xf64( &F[2][i][j][k], gvl );
            vA2 = __builtin_epi_vfmul_1xf64(vF2, v_invM, gvl);
            __builtin_epi_vstore_1xf64(&A[2][i][j][k], vA2, gvl);

            k+=gvl;
	 }

}

#undef COLAPSED
#define COLAPSED

void vel_intr(int coreid, int ncores, int n, double (*V)[n][n][n], double (*A)[n][n][n], double dt)
{
   int i, j, k;
   unsigned long rvl, gvl;
   __epi_1xf64 vV0, vV1, vV2, vA0, vA1, vA2;
  
//#ifndef COLAPSED 
#if 1 
  int chunk=n*n*n/ncores;
  int start=coreid*(chunk);
  int end = (coreid==ncores-1) ?  n*n*n : start+chunk;
   for (int cid = start; cid<end; ) {
      i = cid/(n*n);
      j = (cid/n)%n;
      k= cid%n;
      rvl = n*n*n-cid;
#else
   int chunk=n/ncores;
   int start=coreid*(chunk);
   int end = (coreid==ncores-1) ?  n : start+chunk;
   MAX = n;
   for (i = start; i<end; i++) {
      for (j = 0; j<n; j++) {
         for (k = 0; k<n; ) {
            rvl = n-k;
#endif
            gvl = __builtin_epi_vsetvl(rvl, __epi_e64, __epi_m1);
            __epi_1xf64 vdt = __builtin_epi_vfmv_v_f_1xf64(dt, gvl);

            vV0 = __builtin_epi_vload_1xf64( &V[0][i][j][k], gvl );
            vA0 = __builtin_epi_vload_1xf64( &A[0][i][j][k], gvl );
	    vV0 = __builtin_epi_vfmacc_1xf64(vV0, vA0, vdt, gvl);
            __builtin_epi_vstore_1xf64(&V[0][i][j][k], vV0, gvl);

            vV1 = __builtin_epi_vload_1xf64( &V[1][i][j][k], gvl );
            vA1 = __builtin_epi_vload_1xf64( &A[1][i][j][k], gvl );
	    vV1 = __builtin_epi_vfmacc_1xf64(vV1, vA1, vdt, gvl);
            __builtin_epi_vstore_1xf64(&V[1][i][j][k], vV1, gvl);
            vV2 = __builtin_epi_vload_1xf64( &V[2][i][j][k], gvl );
            vA2 = __builtin_epi_vload_1xf64( &A[2][i][j][k], gvl );
	    vV2 = __builtin_epi_vfmacc_1xf64(vV2, vA2, vdt, gvl);
            __builtin_epi_vstore_1xf64(&V[2][i][j][k], vV2, gvl);

//#ifndef COLAPSED
#if 0
            k+=gvl;
         }
      }
   }
#else
      cid+=gvl;
   }
#endif

}

void pos_intr(int coreid, int ncores, int n, double (*X)[n][n][n], double (*V)[n][n][n], double dt)
{
   int i, j, k; 
   unsigned long int rvl, gvl;
   __epi_1xf64 vV0, vV1, vV2, vX0, vX1, vX2;
  
  int chunk=n/ncores;
  int start=coreid*(chunk);
  int end = (coreid==ncores-1) ?  n : start+chunk;


   for (i = start; i<end; i++) {
      for (j = 0; j<n; j++) {
         for (k = 0; k<n;) {
            rvl = n-k;
            gvl = __builtin_epi_vsetvl(rvl, __epi_e64, __epi_m1);
            __epi_1xf64 vdt = __builtin_epi_vfmv_v_f_1xf64(dt, gvl);

            vX0 = __builtin_epi_vload_1xf64( &X[0][i][j][k], gvl );
            vV0 = __builtin_epi_vload_1xf64( &V[0][i][j][k], gvl );
	    vX0 = __builtin_epi_vfmacc_1xf64(vX0, vV0, vdt, gvl);
            __builtin_epi_vstore_1xf64(&X[0][i][j][k], vX0, gvl);

            vX1 = __builtin_epi_vload_1xf64( &X[1][i][j][k], gvl );
            vV1 = __builtin_epi_vload_1xf64( &V[1][i][j][k], gvl );
	    vX1 = __builtin_epi_vfmacc_1xf64(vX1, vV1, vdt, gvl);
            __builtin_epi_vstore_1xf64(&X[1][i][j][k], vX1, gvl);
            vX2 = __builtin_epi_vload_1xf64( &X[2][i][j][k], gvl );
            vV2 = __builtin_epi_vload_1xf64( &V[2][i][j][k], gvl );
	    vX2 = __builtin_epi_vfmacc_1xf64(vX2, vV2, vdt, gvl);
            __builtin_epi_vstore_1xf64(&X[2][i][j][k], vX2, gvl);

            k+=gvl;
         }
      }
   }
   // would need to check that possition des not go beyond the box walls
}
//      for (i = 0; i<N; i++)
//         for (j = 0; j<N; j++)
//            for (kk = 0; kk<N; kk+=vl) {

//             int maxk = (kk+vl < N? kk+vl: N);
//               for (k = kk; k<maxk; k++) {
//               X[0][i][j][k] += V[0][i][j][k]*dt;
//               X[1][i][j][k] += V[1][i][j][k]*dt;
//               X[2][i][j][k] += V[2][i][j][k]*dt;
//             }
//            }

