#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "util.h"

#include "dataset.h"

//#define CHECK_RESULTS


// 0 -> EPI
// 1 -> MANUAL
// 2 -> PACKED
#define ALGORITHM 2

void SpMV_ref(double *a, long *ia, long *ja, double *x, double *y, int nrows) {
  int row, idx;
  for (row = 0; row < nrows; row++) {
    double sum = 0.0;
    for (idx = ia[row]; idx < ia[row + 1]; idx++) {
      sum += a[idx] * x[ja[idx]];
    }
    y[row] = sum;
  }
}

void SpMV_vector(int coreid, int ncores, double *a, long *ia, long *ja, double *x, double *y, int nrows) {
  printf("In SpMV EPI\n");
  
  int chunk=nrows/ncores;
  int start=coreid*(chunk);
  int end = (coreid==ncores-1) ?  nrows : start+chunk;
  for (int row = start; row < end; row++) {
    // requested vector length (rvl)
    int rvl = ia[row + 1] - ia[row];
    // granted vector length (gvl)
    int gvl = __builtin_epi_vsetvl(rvl, __epi_e64, __epi_m1);
    // This algorithm assumes gvl == rvl
    //assert(rvl == gvl);

    int idx = ia[row];
    __epi_1xf64 va = __builtin_epi_vload_1xf64(&a[idx], gvl);

    // We need to scale it by the size of the double
    __epi_1xi64 v_idx_row = __builtin_epi_vload_1xi64(&ja[idx], gvl);
    __epi_1xi64 vthree = __builtin_epi_vmv_v_x_1xi64(3, gvl);
    v_idx_row = __builtin_epi_vsll_1xi64(v_idx_row, vthree, gvl);

    __epi_1xf64 vx = __builtin_epi_vload_indexed_1xf64(x, v_idx_row, gvl);
    __epi_1xf64 vprod = __builtin_epi_vfmul_1xf64(va, vx, gvl);
    __epi_1xf64 vzero = __builtin_epi_vfmv_v_f_1xf64(0.0, gvl);
    __epi_1xf64 vsum = __builtin_epi_vfredsum_1xf64(vprod, vzero, gvl);
    //y[row] = __builtin_epi_vgetfirst_1xf64(vsum, gvl);
    y[row] = __builtin_epi_vfmv_f_s_1xf64(vsum);
  }
}

void SpMV_vector_manual(
  int coreid,
  int ncores,
  double *Values,			// a0 pointer
  long *Row_ptr,			// a1
  long *Col_ind,			// a2
  double X[],					// a3
  double *Y,					// a4
  long nrows)					// a5
{
  printf("In SpMV Manual\n");
  int last = Row_ptr[0];
  int chunk=nrows/ncores;
  int start=coreid*(chunk);
  int end = (coreid==ncores-1) ?  nrows : start+chunk;
  register double * xv = X;

  int cur_nnz=0;
  for (long i=start; i< end; i++)  {
      printf("Iteration %d out of %d\n", i, end);
      double * cur_vals = Values+cur_nnz;
      long * cur_inds = Col_ind+cur_nnz;
      cur_nnz = Row_ptr[i+1]-Row_ptr[i];

      // Vector loop of:
      // for (int j=0; j< cur_nnz; j++)
      //   sum += cur_vals[j]*xv[cur_inds[j]];
      double sum_in = 0.0;
      double sum_out;


      asm volatile(
              "mv           t0,%2\n"
              "mv           t1,%3\n"
              "mv           t2,%4\n"
              "vsetvli	t3,zero,e64,m8\n"		  // LMUL=8 gouups of 8 vector regs
              "vmv.v.x			v24,zero\n"           // splat 0.0 to v24-31
              "9:\n"
              "vsetvli		  t3,t2,e32,m8\n"
              "vlw.v			v8,(t1)\n"				    // Load cur_inds[] vector (int data) //BORJA: When you load something smaller than the element size it is automatically sign extended
              "vsetvli		  t3,t2,e64,m8\n"
              "vsll.vi			v8,v8,3\n"					  // Scale indicies to 8-byte size
              "vlxe.v		v8,(%1),v8\n"			  	// Indexed load v8-15=X[] vector
              "vle.v			v16,(t0)\n"					  // Load cur_vals vector (double data)
              "vfmacc.vv   v24,v8,v16\n"				  // v8-15 += cur_vals[] * xv[ cur_inds[] ]
              "slli			   	t4,t3,2\n"            // t4 = vl*4
              "add			   	t1,t1,t4\n"  				  // cur_inds += vl elements
              "slli			   	t4,t3,3\n"            // t4 = vl*8
              "add			   	t0,t0,t4\n"  				  // cur_vals += vl elements
              "sub				  t2,t2,t3\n" 					// Subtract number done
              "bnez			  	t2,9b\n" 			        // Any more in vector?
              "vsetvli		  t3,zero,e64,m8\n"     // Maximum length vector
              "vfmv.s.f     v16,%5\n"             // v16[0] = sum_in
              "vfredsum.vs  v8,v24,v16\n"         // Final reduction sum to v8[0]
              "vfmv.f.s			%0,v8\n"              // sum_out = v8[0]
              : "=f"(sum_out)
              : "r"(xv), "r"(cur_vals), "r"(cur_inds), "r"(cur_nnz), "f"(sum_in)
                    : "t0", "t1", "t2", "t3", "t4" );

      Y[i] = sum_out;
      printf("\t res is %d\n", (int)sum_out);
  }
  printf("Out of loop\n");
}

void SpMV_vector_packed(int coreid, int ncores, double *a, long *ia, long *ja, double *x, double *y, long nrows) {

  printf("In SpMV packed\n");

  unsigned long gvl, shvl, red_rvl = 1, red_gvl; // requested & granted vector lengths
  unsigned long MAXVL;
  unsigned long row = 0;
  unsigned long nnz = ia[nrows] - ia[0];
  unsigned long nnz_row = ia[row + 1] - ia[row];
  __epi_1xf64 partial_res;
  MAXVL = __builtin_epi_vsetvlmax(__epi_e64, __epi_m1);
  partial_res = __builtin_epi_vfmv_v_f_1xf64(0.0, MAXVL);

//  y[0] = 0.0;
  int chunk=nnz/ncores;
  int start=coreid*(chunk);
  int end = (coreid==ncores-1) ?  nnz : start+chunk;
  for (unsigned long col = start; col < nnz; ) { // dynamic blocking of a and ja
    gvl = __builtin_epi_vsetvl(nnz - col, __epi_e64, __epi_m1);
    shvl = gvl;
    __epi_1xf64 v_a = __builtin_epi_vload_1xf64(&a[col], gvl);
    // We need to scale it by the size of the double
    __epi_1xi64 v_idx_row = __builtin_epi_vload_1xi64(&ja[col], gvl);
    __epi_1xi64 v_three = __builtin_epi_vmv_v_x_1xi64(3, gvl);
    v_idx_row = __builtin_epi_vsll_1xi64(v_idx_row, v_three, gvl);

    __epi_1xf64 v_x = __builtin_epi_vload_indexed_1xf64(x, v_idx_row, gvl);
    __epi_1xf64 v_prod = __builtin_epi_vfmul_1xf64(v_a, v_x, gvl);

    // /*
//    for (int col_local = 0; col_local < gvl; col_local += red_rvl) {
    for (int col_local = 0; col_local < gvl; ) {
      red_rvl = (col_local + nnz_row) > gvl ? gvl - col_local : nnz_row;
//      red_gvl = red_rvl;
      red_gvl = __builtin_epi_vsetvl(red_rvl, __epi_e64, __epi_m1);
      partial_res = __builtin_epi_vfredsum_1xf64(v_prod, partial_res, red_gvl);
      nnz_row -= red_gvl;

      if (nnz_row == 0) {
        y[row] = __builtin_epi_vfmv_f_s_1xf64(partial_res);
        row++;
//        y[row] = 0.0;
        // TOFIX: not needed last time
        if (row < nrows) {
          partial_res = __builtin_epi_vfmv_v_f_1xf64(0.0, MAXVL);
          nnz_row = ia[row + 1] - ia[row];
        }
      }

      shvl -= red_gvl;
      unsigned long gshvl = __builtin_epi_vsetvl(shvl, __epi_e64, __epi_m1);
      v_prod = __builtin_epi_vslidedown_1xf64(v_prod, red_gvl, gshvl);

      col_local += red_gvl;
    }
    col += gvl;
  }
}

void capture_ref_result(double *y, double *y_ref, int nrows) {
  int row;
  // printf ("\nReference result: ");
  for (row = 0; row < nrows; row++) {
    y_ref[row] = y[row];
    // printf (" %e", y[row]);
  }
  // printf ("\n\n");
}

void test_result(double *y, double *y_ref, int nrows) {
  int row;
  /* Compute with the result to keep the compiler for marking the code as dead
   */
  int success=1;
  for (row = 0; row < nrows; row++) {
    if (y[row] != y_ref[row]) {
      success=0;
      printf("y[%d]=%d != y_ref[%d]=%d\nINCORRECT RESULT !!!!\n", row, (int)y[row],
             row, (int)y_ref[row]);
      break;
    }
  }
  if(success)
  {
    printf("CORRECT RESULTS!!\n");
  }
}

int thread_entry(int cid, int nc)
{
  
  int n, nnzMax, nrows;

  /* Warning: if n > sqrt(2^31), you may get integer overflow */

  /* Allocate enough storage for the matix.  We allocate more than
     is needed in order to simplify the code */
  nrows = N_ROWS;
  nnzMax = nrows * 5;
      
  static double y[N_ROWS];

  barrier(nc);


  if(ALGORITHM==0)
  {
    SpMV_vector(cid, nc, a, ia, ja, x, y, nrows);
  }
  else if(ALGORITHM==1)
  {
    SpMV_vector_manual(cid, nc, a, ia, ja, x, y, nrows);
  }
  else if(ALGORITHM==2)
  {
    SpMV_vector_packed(cid, nc, a, ia, ja, x, y, nrows);
  }
  else
  {
    printf("Unsupported algorithm\n");
  }

  barrier(nc);

  #ifdef CHECK_RESULTS
  if(cid==0)
  {
      double y_ref[DIM];
      SpMV_ref(a, ia, ja, x, y_ref, nrows);
      test_result(y, y_ref, nrows);
  }
  barrier(nc);
  #endif

  if(cid==0)
  {
      exit(1);
  }
}
