// See LICENSE for license details.

//**************************************************************************
// Vector-vector add benchmark
//--------------------------------------------------------------------------
// Author  : Andrew Waterman
// TA      : Christopher Celio
// Student : 
//
// This benchmark adds two vectors and writes the results to a
// third vector. The input data (and reference data) should be
// generated using the vvadd_gendata.pl perl script and dumped
// to a file named dataset.h 

//--------------------------------------------------------------------------
// Includes 

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


//--------------------------------------------------------------------------
// Input/Reference Data

#include "dataset.h"
 
  
//--------------------------------------------------------------------------
// Basic Utilities and Multi-thread Support

#include "util.h"
   
 

extern void __attribute__((noinline)) matmul(int coreid, int ncores, size_t n, const data_t* x, const data_t* y, data_t* z);


//--------------------------------------------------------------------------
// Main
//
// all threads start executing thread_entry(). Use their "coreid" to
// differentiate between threads (each thread is running on a separate core).
  
void thread_entry(int cid, int nc)
{
    // static allocates data in the binary, which is visible to both threads
    static data_t results_data[ARRAY_SIZE];

    // First do out-of-place vvadd
    barrier(nc);
    matmul(cid, nc, DIM_SIZE, input1_data, input2_data, results_data); 
    barrier(nc);//This barrier is necessary for all the simulated cores to finish

    if(cid==0)
    {
        exit(1); //We have to exit with something different from 0
    }
}
