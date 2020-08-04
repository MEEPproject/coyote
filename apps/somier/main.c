#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>
#include "util.h"
#include "somier_utils.h"
#include "somier.h"

#include "dataset.h"


double dt=0.001;   // 0.1;
double spring_K=10.0;
double M=1.0;
int err;


int thread_entry(int cid, int nc)
{
   int ntsteps;
ntsteps = 10;

   printf ("Problem size = %d, steps = %d\n", N, ntsteps);

   double (*F_ref)[N][N][N];

   printf("Set initial speed\n");
//   V[2][N/2][N/2][N/2] = 0.1;  
//   V[1][N/2][N/2][N/2] = 0.1;  
//   V[0][N/2][N/2][N/2] = 0.1;  V[1][N/2][N/2][N/2] = 0.1;  V[2][N/2][N/2][N/2] = 0.1;  
//   print_prv_header();
   //V[0][N/2][N/2][N/2] = 0.1;  V[1][N/2][N/2][N/2] = 0.1;  V[2][N/2][N/2][N/2] = 0.1;


   int nt;
   for (nt=0; nt <ntsteps-1; nt++) {
      if(nt%10 == 0) {
        print_state (N, X, Xcenter, nt);
      }

//      boundary(N, X, V);
//      printf ("\nCorrected Positions\n"); print_4D(N, "X", X); printf ("\n");
      Xcenter[0]=0, Xcenter[1]=0; Xcenter[2]=0;   //reset aggregate stats
      clear_4D(N, F);

#ifdef SEQ
      compute_forces(N, X, F);
      acceleration(N, A, F, M);
      velocities(N, V, A, dt);
      positions(N, X, V, dt);
#else
      compute_forces_prevec(cid, nc, N, X, F); // printf ("Computed forces\n"); print_4D(N, "F", F); printf ("\n");
      barrier(nc);
      accel_intr  (cid, nc, N, A, F, M);       // printf ("Computed Accelerations\n"); print_4D(N, "A", A); printf ("\n");
      barrier(nc);
      vel_intr  (cid, nc, N, V, A, dt);        // printf ("Computed Velocities\n"); print_4D(N, "V", V); printf ("\n");
      barrier(nc);
//      vel_intr  (N, X, V, dt);        // printf ("Computed Positions\n"); print_4D(N, "X", X); printf ("\n");
      pos_intr  (cid, nc, N, X, V, dt);        // printf ("Computed Positions\n"); print_4D(N, "X", X); printf ("\n");
      barrier(nc);

#endif
      compute_stats(N, X, Xcenter);
//      print_prv_record();
   }
	//printf ("\tV= %f, %f, %f\t\t X= %f, %f, %f\n",
		//V[0][N/2][N/2][N/2], V[1][N/2][N/2][N/2], V[2][N/2][N/2][N/2], 
	        //X[0][N/2][N/2][N/2], X[1][N/2][N/2][N/2], X[2][N/2][N/2][N/2]);
}
