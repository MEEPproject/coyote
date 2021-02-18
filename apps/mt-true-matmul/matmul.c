#include "stdlib.h"
#include "dataset.h"

#define BLOCK_SIZE 4


//PLEASE, USE POWERS OF 2 for n and BLOCK_SIZE

void __attribute__((noinline)) matmul(int coreid, int ncores, size_t n, data_t (* a)[n], data_t (* b)[n], data_t (* c)[n])
{
	size_t i;

	int chunk=n/ncores;
	int start=coreid*(chunk);
	int end = (coreid==ncores-1) ?  n : start+chunk;

    for(int ii=start; ii<end; ii+=BLOCK_SIZE)
    {
        for(int jj=0; jj<n; jj+=BLOCK_SIZE)
        {
            for(int kk=0; kk<n; kk+=BLOCK_SIZE)
            {
                for(int i=0; i<BLOCK_SIZE; i++)
                {
                    for(int j=0; j<BLOCK_SIZE; j++)
                    {
                        for(int k=0; k<BLOCK_SIZE; k++)
                        {
                            c[i+ii][j+jj] += a[i+ii][k+kk]*b[k+kk][j+jj];
                        }
                    }
                }
            }
        }
    }
}
