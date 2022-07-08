#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <string.h>
#include "dataset.h"
#include "util.h"

#define OVERLAP_COMPUTE_AND_LOAD
#define REORDER_FLDS

#if 0
#define FMA( vc, vb, sa, gvl ) do { \
  __epi_1xf64 vta  = __builtin_epi_vfmv_v_f_1xf64(sa, gvl); \
  __epi_1xf64 vtp  = __builtin_epi_vfmul_1xf64(vta, vb, gvl); \
  vc   = __builtin_epi_vfadd_1xf64(vc, vtp, gvl); \
} while(0)
#endif

#define FMA( vc, vb, sa, gvl ) do { \
  __epi_1xf64 vta  = __builtin_epi_vfmv_v_f_1xf64(sa, gvl); \
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
     unsigned long gvl = __builtin_epi_vsetvl(end-jj, __epi_e64, __epi_m1); // system selected columm block size (C & B)
     for (ii = 0; ii < M-15; ii += bi) {                        //unroll rows (no possibility of indirection on register addressing)
		__epi_1xf64 vc0, vc1, vc2, vc3, vc4, vc5, vc6, vc7, vc8, vc9, vc10, vc11, vc12, vc13, vc14, vc15;
        __epi_1xf64 vb0;

#ifdef OVERLAP_COMPUTE_AND_LOAD
        __epi_1xf64 vb1;
        double fld0;
        double fld1;
        double fld2;
        double fld3;
        double fld4;
        double fld5;
        double fld6;
        double fld7;
        double fld8;
        double fld9;
        double fld10;
        double fld11;
        double fld12;
        double fld13;
        double fld14;
        double fld15;
        double fld16;
        double fld17;
        double fld18;
        double fld19;
        double fld20;
        double fld21;
        double fld22;
        double fld23;
        double fld24;
        double fld25;
        double fld26;
        double fld27;
        double fld28;
        double fld29;
        double fld30;
        double fld31;
#endif


        vc0  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl);

#if 0
	vc1=vc2=vc3=vc4=vc5=vc6=vc7=vc8=vc9=vc10=vc11=vc12=vc13=vc14=vc15=vc0;
#else
        vc1  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc2 = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc3  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc4  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc5  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc6  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc7  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc8  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc9  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc10  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc11  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc12  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc13  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc14  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc15  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
#endif

#ifdef OVERLAP_COMPUTE_AND_LOAD
        // Load for kk=0
        vb1 = __builtin_epi_vload_1xf64(&b[0][jj], gvl);
        fld16=a[ii][0];
        fld17=a[ii+1][0];
        fld18=a[ii+2][0];
        fld19=a[ii+3][0];
        fld20=a[ii+4][0];
        fld21=a[ii+5][0];
        fld22=a[ii+6][0];
        fld23=a[ii+7][0];
        fld24=a[ii+8][0];
        fld25=a[ii+9][0];
        fld26=a[ii+10][0];
        fld27=a[ii+11][0];
        fld28=a[ii+12][0];
        fld29=a[ii+13][0];
        fld30=a[ii+14][0];
        fld31=a[ii+15][0];
#endif


        for (int kk = 0; kk < K; kk += bk) {
#ifdef OVERLAP_COMPUTE_AND_LOAD
         if (kk + bk < K) 
         {
           vb0=vb1;
           vb1 = __builtin_epi_vload_1xf64(&b[kk+bk][jj], gvl);
           fld0=fld16;
           fld16=a[ii][kk+bk];
           FMA( vc0,  vb0,  fld0, gvl ); //vc0 calculates the first  value of row0
           fld1=fld17;
           fld17=a[ii+1][kk+bk];
           FMA( vc1,  vb0,  fld1, gvl ); //vc1 calculates the first  value of row1
           fld2=fld18;
           fld18=a[ii+2][kk+bk];
           FMA( vc2,  vb0,  fld2, gvl );
           fld3=fld19;
           fld19=a[ii+2][kk+bk];
           FMA( vc3,  vb0,  fld3, gvl );
           fld4=fld20;
           fld20=a[ii+2][kk+bk];
           FMA( vc4,  vb0,  fld4, gvl );
           fld5=fld21;
           fld21=a[ii+2][kk+bk];
           FMA( vc5,  vb0,  fld5, gvl );
           fld6=fld22;
           fld22=a[ii+2][kk+bk];
           FMA( vc6,  vb0,  fld6, gvl );
           fld7=fld23;
           fld23=a[ii+2][kk+bk];
           FMA( vc7,  vb0,  fld7, gvl );
           fld8=fld24;
           fld24=a[ii+2][kk+bk];
           FMA( vc8,  vb0,  fld8, gvl );
           fld9=fld25;
           fld25=a[ii+2][kk+bk];
           FMA( vc9,  vb0,  fld9, gvl );
           fld10=fld26;
           fld26=a[ii+2][kk+bk];
           FMA( vc10, vb0,  fld10, gvl );
           fld11=fld27;
           fld27=a[ii+2][kk+bk];
           FMA( vc11, vb0,  fld11, gvl );
           fld12=fld28;
           fld28=a[ii+2][kk+bk];
           FMA( vc12, vb0,  fld12, gvl );
           fld13=fld29;
           fld29=a[ii+2][kk+bk];
           FMA( vc13, vb0,  fld13, gvl );
           fld14=fld30;
           fld30=a[ii+2][kk+bk];
           FMA( vc14, vb0,  fld14, gvl );
           fld15=fld31;
           fld31=a[ii+2][kk+bk];
           FMA( vc15, vb0,  fld15, gvl );
          }
         else
         { // Last elements
           FMA( vc0,  vb1,  fld16, gvl ); //vc0 calculates the first  value of row0
           FMA( vc1,  vb1,  fld17, gvl ); //vc1 calculates the first  value of row1
           FMA( vc2,  vb1,  fld18, gvl );   
           FMA( vc3,  vb1,  fld19, gvl );    
           FMA( vc4,  vb1,  fld20, gvl );   
           FMA( vc5,  vb1,  fld21, gvl );   
           FMA( vc6,  vb1,  fld22, gvl );         
           FMA( vc7,  vb1,  fld23, gvl );      
           FMA( vc8,  vb1,  fld24, gvl );           
           FMA( vc9,  vb1,  fld25, gvl );     
           FMA( vc10, vb1,  fld26, gvl );         
           FMA( vc11, vb1,  fld27, gvl );         
           FMA( vc12, vb1,  fld28, gvl );        
           FMA( vc13, vb1,  fld29, gvl );        
           FMA( vc14, vb1,  fld30, gvl );         
           FMA( vc15, vb1,  fld31, gvl );

         }

#else

           vb0 = __builtin_epi_vload_1xf64(&b[kk][jj], gvl);
	   

           FMA( vc0,  vb0,   a[ii][kk], gvl ); //vc0 calculates the first  value of row0
           FMA( vc1,  vb0,  a[ii+1][kk], gvl ); //vc1 calculates the first  value of row1
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
#endif
	   
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
     unsigned long int gvl = __builtin_epi_vsetvl(end-jj, __epi_e64, __epi_m1);
     for (ii=ii_left; ii < M; ii += 2) {
        vc0  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc1  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc2  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 
        vc3  = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl); 

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
  simfence();

  #ifdef CHECK_RESULTS
  int res = verifyDouble(ARRAY_SIZE, results_data, verify_data);
  if(cid==0)
  {
    if(res==0)
    {
      printf("\e[32mTest pass\e[0m\n");
    } else
    {
      printf("\e[31mTest fails on position %d\e[0m\n", res);
    }
  }
  simfence();
  #endif

  if(cid==0)
  {
	  exit(results_data[0]);
  }
}
