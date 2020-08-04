#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>
#include "../somier.h"
#include "somier_v.h"



static int count=0;

//reference scalar code 
void force_contr_prevec(int n, double (*X)[n][n][n], double (*F)[n][n][n], int i, int j, int neig_i, int neig_j)
{
   double dx, dy, dz, dl, spring_F, FX, FY,FZ;

   for (int k=1; k<n-1; k++) {

      dx=X[0][neig_i][neig_j][k]-X[0][i][j][k];
      dy=X[1][neig_i][neig_j][k]-X[1][i][j][k];
      dz=X[2][neig_i][neig_j][k]-X[2][i][j][k];
      dl = sqrt(dx*dx + dy*dy + dz*dz);
      spring_F = 0.25 * spring_K*(dl-1);
      FX = spring_F * dx/dl; 
      FY = spring_F * dy/dl;
      FZ = spring_F * dz/dl; 
      F[0][i][j][k] += FX;
      F[1][i][j][k] += FY;
      F[2][i][j][k] += FZ;
   }
}

   double buffer[1024];

inline void force_contr_vec(int n, double (*X)[n][n][n], double (*F)[n][n][n], int i, int j, int neig_i, int neig_j)
{
   unsigned long gvl = __builtin_epi_vsetvl(n, __epi_e64, __epi_m1);

   __epi_1xf64 v_1        = __builtin_epi_vfmv_v_f_1xf64(1.0, gvl);
   __epi_1xf64 v_spr_K    = __builtin_epi_vfmv_v_f_1xf64(0.25*spring_K, gvl);

   for (int k=1; k<n-1; ) {

      gvl = __builtin_epi_vsetvl(n-1 - k, __epi_e64, __epi_m1);

      __epi_1xf64 v_x1    = __builtin_epi_vload_1xf64(&X[0][neig_i][neig_j][k], gvl);
      __epi_1xf64 v_x2    = __builtin_epi_vload_1xf64(&X[0][i][j][k], gvl);
      __epi_1xf64 v_dx    = __builtin_epi_vfsub_1xf64(v_x1, v_x2, gvl);
      __epi_1xf64 v_dx2   = __builtin_epi_vfmul_1xf64(v_dx, v_dx, gvl);
      __epi_1xf64 v_x3    = __builtin_epi_vload_1xf64(&X[1][neig_i][neig_j][k], gvl);
      __epi_1xf64 v_x4    = __builtin_epi_vload_1xf64(&X[1][i][j][k], gvl);
      __epi_1xf64 v_dy    = __builtin_epi_vfsub_1xf64(v_x3, v_x4, gvl);
      __epi_1xf64 v_dy2   = __builtin_epi_vfmul_1xf64(v_dy, v_dy, gvl);
      __epi_1xf64 v_x5    = __builtin_epi_vload_1xf64(&X[2][neig_i][neig_j][k], gvl);
      __epi_1xf64 v_x6    = __builtin_epi_vload_1xf64(&X[2][i][j][k], gvl);
      __epi_1xf64 v_dz    = __builtin_epi_vfsub_1xf64(v_x5, v_x6, gvl);
      __epi_1xf64 v_dz2   = __builtin_epi_vfmul_1xf64(v_dz, v_dz, gvl);
      __epi_1xf64 v_dl    = __builtin_epi_vfadd_1xf64(v_dx2, v_dy2, gvl);
      v_dl                = __builtin_epi_vfadd_1xf64(v_dl, v_dz2, gvl);
      v_dl                = __builtin_epi_vfsqrt_1xf64(v_dl, gvl);
      __epi_1xf64 v_dl1   = __builtin_epi_vfsub_1xf64(v_dl, v_1, gvl);
      __epi_1xf64 v_spr_F = __builtin_epi_vfmul_1xf64(v_spr_K, v_dl1, gvl);
      __epi_1xf64 v_dFX = __builtin_epi_vfdiv_1xf64(v_dx, v_dl, gvl);
      __epi_1xf64 v_dFY = __builtin_epi_vfdiv_1xf64(v_dy, v_dl, gvl);
      __epi_1xf64 v_dFZ = __builtin_epi_vfdiv_1xf64(v_dz, v_dl, gvl);
      __epi_1xf64 v_FX    = __builtin_epi_vload_1xf64( &F[0][i][j][k], gvl );
      __epi_1xf64 v_FY    = __builtin_epi_vload_1xf64( &F[1][i][j][k], gvl );
      __epi_1xf64 v_FZ    = __builtin_epi_vload_1xf64( &F[2][i][j][k], gvl );
      v_FX               = __builtin_epi_vfmacc_1xf64(v_FX, v_spr_F, v_dFX,  gvl);
      v_FY               = __builtin_epi_vfmacc_1xf64(v_FY, v_spr_F, v_dFY,  gvl);
      v_FZ               = __builtin_epi_vfmacc_1xf64(v_FZ, v_spr_F, v_dFZ,  gvl);
      __builtin_epi_vstore_1xf64(&F[0][i][j][k], v_FX, gvl);
      __builtin_epi_vstore_1xf64(&F[1][i][j][k], v_FY, gvl);
      __builtin_epi_vstore_1xf64(&F[2][i][j][k], v_FZ, gvl);

      k += gvl;
   }
}

void k_force_contr_vec(int n, double (*X)[n][n][n], double (*F)[n][n][n], int i, int j)
{
   long gvl = __builtin_epi_vsetvl(n, __epi_e64, __epi_m1);

   __epi_1xf64 v_1        = __builtin_epi_vfmv_v_f_1xf64(1.0, gvl);
   __epi_1xf64 v_spr_K    = __builtin_epi_vfmv_v_f_1xf64(0.25*spring_K, gvl);

   for (int k=1; k<n-1; ) {

      gvl = __builtin_epi_vsetvl(n-1 - k, __epi_e64, __epi_m1);

      // Still missing: elements to shift at the boundaries !!!!!
     
      __epi_1xf64 v_x    = __builtin_epi_vload_1xf64(&X[0][i][j][k], gvl);
      __epi_1xf64 v_y    = __builtin_epi_vload_1xf64(&X[1][i][j][k], gvl);
      __epi_1xf64 v_z    = __builtin_epi_vload_1xf64(&X[2][i][j][k], gvl);
      __epi_1xf64 v_FX   = __builtin_epi_vload_1xf64( &F[0][i][j][k], gvl );
      __epi_1xf64 v_FY   = __builtin_epi_vload_1xf64( &F[1][i][j][k], gvl );
      __epi_1xf64 v_FZ   = __builtin_epi_vload_1xf64( &F[2][i][j][k], gvl );

      __epi_1xf64 v_xs    = __builtin_epi_vslidedown_1xf64(v_x, 1, gvl) ;
      __epi_1xf64 v_ys    = __builtin_epi_vslidedown_1xf64(v_y, 1, gvl) ;
      __epi_1xf64 v_zs    = __builtin_epi_vslidedown_1xf64(v_z, 1, gvl) ;
      __epi_1xf64 v_dx    = __builtin_epi_vfsub_1xf64(v_xs, v_x, gvl);
      __epi_1xf64 v_dx2   = __builtin_epi_vfmul_1xf64(v_dx, v_dx, gvl);
      __epi_1xf64 v_dy    = __builtin_epi_vfsub_1xf64(v_ys, v_y, gvl);
      __epi_1xf64 v_dy2   = __builtin_epi_vfmul_1xf64(v_dy, v_dy, gvl);
      __epi_1xf64 v_dz    = __builtin_epi_vfsub_1xf64(v_zs, v_z, gvl);
      __epi_1xf64 v_dz2   = __builtin_epi_vfmul_1xf64(v_dz, v_dz, gvl);
      __epi_1xf64 v_dl    = __builtin_epi_vfadd_1xf64(v_dx2, v_dy2, gvl);
      v_dl                = __builtin_epi_vfadd_1xf64(v_dl, v_dz2, gvl);
      v_dl                = __builtin_epi_vfsqrt_1xf64(v_dl, gvl);
      __epi_1xf64 v_dl1   = __builtin_epi_vfsub_1xf64(v_dl, v_1, gvl);
      __epi_1xf64 v_spr_F = __builtin_epi_vfmul_1xf64(v_spr_K, v_dl1, gvl);
      __epi_1xf64 v_dFX = __builtin_epi_vfdiv_1xf64(v_dx, v_dl, gvl);
      __epi_1xf64 v_dFY = __builtin_epi_vfdiv_1xf64(v_dy, v_dl, gvl);
      __epi_1xf64 v_dFZ = __builtin_epi_vfdiv_1xf64(v_dz, v_dl, gvl);
      v_FX               = __builtin_epi_vfmacc_1xf64(v_FX, v_spr_F, v_dFX,  gvl);
      v_FY               = __builtin_epi_vfmacc_1xf64(v_FY, v_spr_F, v_dFY,  gvl);
      v_FZ               = __builtin_epi_vfmacc_1xf64(v_FZ, v_spr_F, v_dFZ,  gvl);

      v_xs    = __builtin_epi_vslideup_1xf64(v_x, 1, gvl) ;
      v_ys    = __builtin_epi_vslideup_1xf64(v_y, 1, gvl) ;
      v_zs    = __builtin_epi_vslideup_1xf64(v_z, 1, gvl) ;
      v_dx    = __builtin_epi_vfsub_1xf64(v_xs, v_x, gvl);
      v_dx2   = __builtin_epi_vfmul_1xf64(v_dx, v_dx, gvl);
      v_dy    = __builtin_epi_vfsub_1xf64(v_ys, v_y, gvl);
      v_dy2   = __builtin_epi_vfmul_1xf64(v_dy, v_dy, gvl);
      v_dz    = __builtin_epi_vfsub_1xf64(v_zs, v_z, gvl);
      v_dz2   = __builtin_epi_vfmul_1xf64(v_dz, v_dz, gvl);
      v_dl    = __builtin_epi_vfadd_1xf64(v_dx2, v_dy2, gvl);
      v_dl    = __builtin_epi_vfadd_1xf64(v_dl, v_dz2, gvl);
      v_dl    = __builtin_epi_vfsqrt_1xf64(v_dl, gvl);
      v_dl1   = __builtin_epi_vfsub_1xf64(v_dl, v_1, gvl);
      v_spr_F = __builtin_epi_vfmul_1xf64(v_spr_K, v_dl1, gvl);
      v_dFX   = __builtin_epi_vfdiv_1xf64(v_dx, v_dl, gvl);
      v_dFY   = __builtin_epi_vfdiv_1xf64(v_dy, v_dl, gvl);
      v_dFZ   = __builtin_epi_vfdiv_1xf64(v_dz, v_dl, gvl);
      v_FX    = __builtin_epi_vfmacc_1xf64(v_FX, v_spr_F, v_dFX,  gvl);
      v_FY    = __builtin_epi_vfmacc_1xf64(v_FY, v_spr_F, v_dFY,  gvl);
      v_FZ    = __builtin_epi_vfmacc_1xf64(v_FZ, v_spr_F, v_dFZ,  gvl);


      __builtin_epi_vstore_1xf64(&F[0][i][j][k], v_FX, gvl);
      __builtin_epi_vstore_1xf64(&F[1][i][j][k], v_FY, gvl);
      __builtin_epi_vstore_1xf64(&F[2][i][j][k], v_FZ, gvl);

      k += gvl;
   }
}


void k_force_contr_prevec(int n, double (*X)[n][n][n], double (*F)[n][n][n], int i, int j)
{
   double dx, dy, dz, dl, spring_F, FX, FY,FZ;

   for (int k=1; k<n-1; k++) {
      dx=X[0][i][j][k-1]-X[0][i][j][k];
      dy=X[1][i][j][k-1]-X[1][i][j][k];
      dz=X[2][i][j][k-1]-X[2][i][j][k];
      dl = sqrt(dx*dx + dy*dy + dz*dz);
      spring_F = 0.25 * spring_K*(dl-1);
      FX = spring_F * dx/dl; 
      FY = spring_F * dy/dl;
      FZ = spring_F * dz/dl; 
      F[0][i][j][k] += FX;
      F[1][i][j][k] += FY;
      F[2][i][j][k] += FZ;
      dx=X[0][i][j][k+1]-X[0][i][j][k];
      dy=X[1][i][j][k+1]-X[1][i][j][k];
      dz=X[2][i][j][k+1]-X[2][i][j][k];
      dl = sqrt(dx*dx + dy*dy + dz*dz);
      spring_F = 0.25 * spring_K*(dl-1);
      FX = spring_F * dx/dl; 
      FY = spring_F * dy/dl;
      FZ = spring_F * dz/dl; 
      F[0][i][j][k] += FX;
      F[1][i][j][k] += FY;
      F[2][i][j][k] += FZ;
   }

}

__attribute__((noinline)) void emit_event()
{
   __asm__("vadd.vi v0,v0,0");
}


void compute_forces_prevec(int coreid, int ncores, int n, double (*X)[n][n][n], double (*F)[n][n][n])
{
   int chunk=(n-2/ncores);
   int start=coreid*(chunk)+1;
   int end = (coreid==ncores-1) ?  n-1 : start+chunk;
   for (int i=start; i<end; i++) {
      for (int j=1; j<n-1; j++) {
            force_contr_vec (n, X, F, i, j, i,   j+1);  
            force_contr_vec (n, X, F, i, j, i-1, j  );   
            force_contr_vec (n, X, F, i, j, i+1, j  );   
            force_contr_vec (n, X, F, i, j, i,   j-1);  
//            force_contr_prevec (n, X, F, i, j, i,   j-1);   //fails if force_contr_vec
            k_force_contr_prevec (n, X, F, i, j);
      }
   }
}

