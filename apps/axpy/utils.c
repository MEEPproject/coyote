#include <assert.h>
#include "utils.h"

void capture_ref_result(int *y, int *y_ref, int n)
{
   int i;
   //printf ("\nReference result: ");
   for (i=0; i<n; i++) {
      y_ref[i]=y[i];
      //printf (" %f", y[i]);
   }
   //printf ("\n\n");
}

void test_result(int *y, int *y_ref, long nrows)
{
   long row;
   int nerrs=0;
   /* Compute with the result to keep the compiler for marking the code as dead */
   for (row=0; row<nrows; row++) {
      double error = y[row] - y_ref[row];
      if (error != 0)  {
         nerrs++;
      }
   }
//   if (nerrs == 0) printf ("Result ok !!!\n");
}



