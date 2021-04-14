#include <assert.h>

void axpy_intrinsics(int coreid, int ncores, int a, double *dx, double *dy, long n) {
  int i;

  int chunk=n/ncores;
  int start=coreid*(chunk);
  int end = (coreid==ncores-1) ?  n : start+chunk;

  long gvl = __builtin_epi_vsetvl(chunk, __epi_e64, __epi_m1);
  __epi_1xi64 v_a = __builtin_epi_vmv_v_x_1xi64(a, gvl);

  for (i = start; i < end;) {
    gvl = __builtin_epi_vsetvl(end-i, __epi_e64, __epi_m1);
    __epi_1xf64 v_dx = __builtin_epi_vload_1xf64(&dx[i], gvl);
    __epi_1xf64 v_dy = __builtin_epi_vload_1xf64(&dy[i], gvl);
    __epi_1xf64 v_res = __builtin_epi_vfmacc_1xf64(v_dy, (__epi_1xf64) v_a, v_dx, gvl);
    __builtin_epi_vstore_1xf64(&dy[i], v_res, gvl);

    i += gvl;
  }
}
