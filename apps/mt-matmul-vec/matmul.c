#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <string.h>
#include "dataset.h"
#include "util.h"

#if 0
#define FMA( vc, vb, sa, gvl ) do { \
  __epi_1xf64 vta  = __builtin_epi_vbroadcast_1xf64(sa, gvl); \
  __epi_1xf64 vtp  = __builtin_epi_vfmul_1xf64(vta, vb, gvl); \
  vc   = __builtin_epi_vfadd_1xf64(vc, vtp, gvl); \
} while(0)
#endif

#define FMA( vc, vb, sa, gvl ) do { \
  __epi_1xf64 vta  = __builtin_epi_vbroadcast_1xf64(sa, gvl); \
  vc  = __builtin_epi_vfmacc_1xf64(vc, vta, vb, gvl); \
} while(0)

static void matmul_builtins(int coreid, int ncores, int M, int K, int N, int bi, int bj, int bk, double (* restrict c)[N],
                     double (* restrict a)[K], double (* restrict b)[N]) {
  assert(bi == 16);
  assert(bk == 1);

  int ii;

  int chunk=N/ncores;
  int start=coreid*(chunk);
  int end = (coreid==ncores-1) ?  N : start+chunk;


  for (int jj = start; jj < end; ) {
     unsigned long gvl = __builtin_epi_vsetvl(N-jj, __epi_e64, __epi_m1); // system selected columm block size (C & B)
     for (ii = 0; ii < M-15; ii += bi) {                        //unroll rows (no possibility of indirection on register addressing)
		__epi_1xf64 vc0, vc1, vc2, vc3, vc4, vc5, vc6, vc7, vc8, vc9, vc10, vc11, vc12, vc13, vc14, vc15;
        __epi_1xf64 vb0;
        vc0  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
#if 0
	vc1=vc2=vc3=vc4=vc5=vc6=vc7=vc8=vc9=vc10=vc11=vc12=vc13=vc14=vc15=vc0;
#else
        vc1  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc2 = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc3  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc4  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc5  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc6  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc7  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc8  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc9  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc10  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc11  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc12  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc13  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc14  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc15  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
#endif

        for (int kk = 0; kk < K; kk += bk) {
           vb0 = __builtin_epi_vload_1xf64(&b[kk][jj], gvl);
	   {
           FMA( vc0,  vb0,   a[ii][kk], gvl );
           FMA( vc1,  vb0,  a[ii+1][kk], gvl );
           FMA( vc2,  vb0,  a[ii+2][kk], gvl );
           FMA( vc3,  vb0,  a[ii+3][kk], gvl );
           FMA( vc4,  vb0,  a[ii+4][kk], gvl );
           FMA( vc5,  vb0,  a[ii+5][kk], gvl );
           FMA( vc6,  vb0,  a[ii+6][kk], gvl );
           FMA( vc7,  vb0,  a[ii+7][kk], gvl );
           FMA( vc8,  vb0,  a[ii+8][kk], gvl );
           FMA( vc9,  vb0,  a[ii+9][kk], gvl );
           FMA( vc10, vb0, a[ii+10][kk], gvl );
           FMA( vc11, vb0, a[ii+11][kk], gvl );
           FMA( vc12, vb0, a[ii+12][kk], gvl );
           FMA( vc13, vb0, a[ii+13][kk], gvl );
           FMA( vc14, vb0, a[ii+14][kk], gvl );
           FMA( vc15, vb0, a[ii+15][kk], gvl );
	   }
        }
        __builtin_epi_vstore_1xf64(&c[ii][jj], vc0, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+1][jj], vc1, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+2][jj], vc2, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+3][jj], vc3, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+4][jj], vc4, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+5][jj], vc5, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+6][jj], vc6, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+7][jj], vc7, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+8][jj], vc8, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+9][jj], vc9, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+10][jj], vc10, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+11][jj], vc11, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+12][jj], vc12, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+13][jj], vc13, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+14][jj], vc14, gvl);
        __builtin_epi_vstore_1xf64(&c[ii+15][jj], vc15, gvl);
     }
    jj += gvl; 
  }
  
  int ii_left=ii;
  for (int jj = start; jj < end; ) {
     __epi_1xf64 vc0, vc1, vc2, vc3;
     __epi_1xf64 vb0;
     unsigned long int gvl = __builtin_epi_vsetvl(N-jj, __epi_e64, __epi_m1);
     for (ii=ii_left; ii < M; ii += 2) {
        vc0  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc1  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc2  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 
        vc3  = __builtin_epi_vbroadcast_1xf64(0.0, gvl); 

        for (int kk = 0; kk < K; kk += bk) {
           vb0 = __builtin_epi_vload_1xf64(&b[kk][jj], gvl);
           {
           FMA( vc0,  vb0,   a[ii][kk], gvl );
           if (ii+1 < M) FMA( vc1,  vb0,   a[ii+1][kk], gvl );
           if (ii+2 < M) FMA( vc2,  vb0,   a[ii+1][kk], gvl );
           if (ii+3 < M) FMA( vc3,  vb0,   a[ii+1][kk], gvl );
           }
        }
        __builtin_epi_vstore_1xf64(&c[ii][jj], vc0, gvl);
        if (ii+1 < M)  __builtin_epi_vstore_1xf64(&c[ii+1][jj], vc1, gvl);
        if (ii+2 < M)  __builtin_epi_vstore_1xf64(&c[ii+2][jj], vc2, gvl);
        if (ii+3 < M)  __builtin_epi_vstore_1xf64(&c[ii+3][jj], vc3, gvl);
     }
     jj += gvl;
  }
}

int thread_entry(int cid, int nc)
{
  // FIXME: Make this a parameter read from the command line
  int m = DIM_SIZE;
  int k = DIM_SIZE;
  int n = DIM_SIZE;

  int bi = 16;
  int bj = 64; //THIS IS THE ONLY ONE OF THE THREE THAT MAY BE CHANGED
  int bk = 1;

  static data_t results_data[ARRAY_SIZE];

  matmul_builtins(cid, nc, m, k, n, bi, bj, bk, results_data, input1_data, input2_data);

  barrier(nc);

  if(cid==0)
  {
	exit(1);
  }
}
