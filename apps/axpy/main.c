#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "util.h"
#include "dataset.h"

void axpy_intrinsics(int coreid, int ncores, int a, double *dx, double *dy, long n);

void axpy_ref(int a, double *dx, double *dy, long n);

void init_vector(double *pv, long n, long value);

/*int __attribute__((naked)) main()
{
    asm ("la sp, _user_stack");
    asm ("j _main");
}*/

int thread_entry(int cid, int nc)
{
    long a = A;
    long n = N;

    // Allocate the source and result vectors
    double dy_ref[n];

    //init_vector(dx, n, 1.0);
    //init_vector(dy, n, 2.0);
    //axpy_ref(a, dx, dy, n);
    //capture_ref_result(dy, dy_ref, n);
    axpy_intrinsics(cid, nc, a, dx, dy, n);
    //test_result(dy, dy_ref, n);

    asm("csrrw t0, 0xc00, t0");
    simfence();

    if(cid==0)
    {
	  exit(0);
    }
    return 0;
}
// Ref version
void axpy_ref(int a, double *dx, double *dy, long n) {
   int i;
   for (i=0; i<n; i++) {
      dy[i] += a*dx[i];
   }
}

void init_vector(double *pv, long n, long value)
{
   for (int i=0; i<n; i++) ((long *)pv)[i] = value;
//   int gvl = __builtin_epi_vsetvl(n, __epi_e64, __epi_m1);
//   __epi_1xi64 v_value   = __builtin_epi_vbroadcast_1xi64((int)value, gvl);
//   for (int i=0; i<n; ) {
//    gvl = __builtin_epi_vsetvl(n - i, __epi_e64, __epi_m1);
//      __builtin_epi_vstore_1xf64(&dx[i], v_res, gvl);
//     i += gvl;
//   }
}

