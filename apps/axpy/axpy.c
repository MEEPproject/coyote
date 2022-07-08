#include <assert.h>

#define _MM_LOAD_f64        __builtin_epi_vload_1xf64
#define _MM_MACC_f64        __builtin_epi_vfmacc_1xf64
#define _MM_STORE_f64       __builtin_epi_vstore_1xf64
#define FENCE()   asm volatile( "fence" : : );
#define _MM_SET_f64         __builtin_epi_vfmv_v_f_1xf64

#define OVERLAP_COMPUTE_AND_LOAD

void axpy_intrinsics(int coreid, int ncores, int a, double *dx, double *dy, long n) {
  int i;

  int chunk=n/ncores;
  int start=coreid*(chunk);
  int end = (coreid==ncores-1) ?  n : start+chunk;

  long gvl = __builtin_epi_vsetvl(chunk, __epi_e64, __epi_m1);
#ifdef OVERLAP_COMPUTE_AND_LOAD
  int j, i0, i1, i2, i3, i4, i5, i6, i7;
  __epi_1xf64 v_a = _MM_SET_f64(a, gvl);
  j=start;
  __epi_1xf64 vc0_dx;
  __epi_1xf64 vc1_dy;
  __epi_1xf64 vc2_dx;
  __epi_1xf64 vc3_dy;
  __epi_1xf64 vc4_dx;
  __epi_1xf64 vc5_dy;
  __epi_1xf64 vc6_dx;
  __epi_1xf64 vc7_dy;
  __epi_1xf64 vc8_dx;
  __epi_1xf64 vc9_dy;
  __epi_1xf64 vc10_dx;
  __epi_1xf64 vc11_dy;
  __epi_1xf64 vc12_dx;
  __epi_1xf64 vc13_dy;
  __epi_1xf64 vc14_dx;
  __epi_1xf64 vc15_dy;
  if (j + 8* gvl < end)
   {
    vc0_dx = _MM_LOAD_f64(&dx[j], gvl);
    vc1_dy = _MM_LOAD_f64(&dy[j], gvl);
    j += gvl;
    vc2_dx = _MM_LOAD_f64(&dx[j], gvl);
    vc3_dy = _MM_LOAD_f64(&dy[j], gvl);
    j += gvl;
    vc4_dx = _MM_LOAD_f64(&dx[j], gvl);
    vc5_dy = _MM_LOAD_f64(&dy[j], gvl);
    j += gvl;
    vc6_dx = _MM_LOAD_f64(&dx[j], gvl);
    vc7_dy = _MM_LOAD_f64(&dy[j], gvl);
    j += gvl;
    vc8_dx = _MM_LOAD_f64(&dx[j], gvl);
    vc9_dy = _MM_LOAD_f64(&dy[j], gvl);
    j += gvl;
    vc10_dx = _MM_LOAD_f64(&dx[j], gvl);
    vc11_dy = _MM_LOAD_f64(&dy[j], gvl);
    j += gvl;
    vc12_dx = _MM_LOAD_f64(&dx[j], gvl);
    vc13_dy = _MM_LOAD_f64(&dy[j], gvl);
    j += gvl;
    vc14_dx = _MM_LOAD_f64(&dx[j], gvl);
    vc15_dy = _MM_LOAD_f64(&dy[j], gvl);
    j += gvl;
   }

    for (i = start; i < end;) 
    {
      if ((j + 8* gvl) < end)
      {
        // 8x copy into vector reg and start load before compute
        __epi_1xf64 vb0_dx = vc0_dx;
        __epi_1xf64 vb1_dy = vc1_dy;
        __epi_1xf64 vc0_dx = _MM_LOAD_f64(&dx[j], gvl);
        __epi_1xf64 vc1_dy = _MM_LOAD_f64(&dy[j], gvl);
        j += gvl;
        vb1_dy = _MM_MACC_f64(vb1_dy, v_a, vb0_dx, gvl);
        _MM_STORE_f64(&dy[i], vb1_dy, gvl);
        i += gvl;

        __epi_1xf64 vb2_dx = vc2_dx;
        __epi_1xf64 vb3_dy = vc3_dy;
        __epi_1xf64 vc2_dx = _MM_LOAD_f64(&dx[j], gvl);
        __epi_1xf64 vc3_dy = _MM_LOAD_f64(&dy[j], gvl);
        j += gvl;
        vb3_dy = _MM_MACC_f64(vb3_dy, v_a, vb2_dx, gvl);
        _MM_STORE_f64(&dy[i], vb3_dy, gvl);
        i += gvl;

        __epi_1xf64 vb4_dx = vc4_dx;
        __epi_1xf64 vb5_dy = vc5_dy;
        __epi_1xf64 vc4_dx = _MM_LOAD_f64(&dx[j], gvl);
        __epi_1xf64 vc5_dy = _MM_LOAD_f64(&dy[j], gvl);
        j += gvl;
        vb5_dy = _MM_MACC_f64(vb5_dy, v_a, vb4_dx, gvl);
        _MM_STORE_f64(&dy[i], vb5_dy, gvl);
        i += gvl;

        __epi_1xf64 vb6_dx = vc6_dx;
        __epi_1xf64 vb7_dy = vc7_dy;
        __epi_1xf64 vc6_dx = _MM_LOAD_f64(&dx[j], gvl);
        __epi_1xf64 vc7_dy = _MM_LOAD_f64(&dy[j], gvl);
        j += gvl;
        vb7_dy = _MM_MACC_f64(vb7_dy, v_a, vb6_dx, gvl);
        _MM_STORE_f64(&dy[i], vb7_dy, gvl);
        i += gvl;

        __epi_1xf64 vb8_dx = vc8_dx;
        __epi_1xf64 vb9_dy = vc9_dy;
        __epi_1xf64 vc8_dx = _MM_LOAD_f64(&dx[j], gvl);
        __epi_1xf64 vc9_dy = _MM_LOAD_f64(&dy[j], gvl);
        j += gvl;
        vb9_dy = _MM_MACC_f64(vb9_dy, v_a, vb8_dx, gvl);
        _MM_STORE_f64(&dy[i], vb9_dy, gvl);
        i += gvl;

        __epi_1xf64 vb10_dx = vc10_dx;
        __epi_1xf64 vb11_dy = vc11_dy;
        __epi_1xf64 vc10_dx = _MM_LOAD_f64(&dx[j], gvl);
        __epi_1xf64 vc11_dy = _MM_LOAD_f64(&dy[j], gvl);
        j += gvl;
        vb11_dy = _MM_MACC_f64(vb11_dy, v_a, vb10_dx, gvl);
        _MM_STORE_f64(&dy[i], vb11_dy, gvl);
        i += gvl;

        __epi_1xf64 vb12_dx = vc12_dx;
        __epi_1xf64 vb13_dy = vc13_dy;
        __epi_1xf64 vc12_dx = _MM_LOAD_f64(&dx[j], gvl);
        __epi_1xf64 vc13_dy = _MM_LOAD_f64(&dy[j], gvl);
        j += gvl;
        vb13_dy = _MM_MACC_f64(vb13_dy, v_a, vb12_dx, gvl);
        _MM_STORE_f64(&dy[i], vb13_dy, gvl);
        i += gvl;

        __epi_1xf64 vb14_dx = vc14_dx;
        __epi_1xf64 vb15_dy = vc15_dy;
        __epi_1xf64 vc14_dx = _MM_LOAD_f64(&dx[j], gvl);
        __epi_1xf64 vc15_dy = _MM_LOAD_f64(&dy[j], gvl);
        j += gvl;
        vb15_dy = _MM_MACC_f64(vb15_dy, v_a, vb14_dx, gvl);
        _MM_STORE_f64(&dy[i], vb15_dy, gvl);
        i += gvl;

      }
      else
      {
        gvl = __builtin_epi_vsetvl(end - i, __epi_e64, __epi_m1);
        __epi_1xf64 v_dx = _MM_LOAD_f64(&dx[i], gvl);
        __epi_1xf64 v_dy = _MM_LOAD_f64(&dy[i], gvl);
        __epi_1xf64 v_res = _MM_MACC_f64(v_dy, v_a, v_dx, gvl);
        _MM_STORE_f64(&dy[i], v_res, gvl);

        i += gvl;
      }
    }


#else
  __epi_1xi64 v_a = __builtin_epi_vmv_v_x_1xi64(a, gvl);
  for (i = start; i < end;) {
    gvl = __builtin_epi_vsetvl(end-i, __epi_e64, __epi_m1);
    __epi_1xf64 v_dx = __builtin_epi_vload_1xf64(&dx[i], gvl);
    __epi_1xf64 v_dy = __builtin_epi_vload_1xf64(&dy[i], gvl);
    __epi_1xf64 v_res = __builtin_epi_vfmacc_1xf64(v_dy, (__epi_1xf64) v_a, v_dx, gvl);
    __builtin_epi_vstore_1xf64(&dy[i], v_res, gvl);

    i += gvl;
  }
#endif
}
