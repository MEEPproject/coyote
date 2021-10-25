#include <stdio.h>
#include <stdlib.h>

#include "util.h"

#include "dataset.h"


void reduction_sum_example(int coreid, int ncores, long *matrix, long *indices, long nElements) {

	long sum_out;

	asm volatile(
			"mv				t1, %2\n"				// load the address to the indices
			"mv				t2, %3\n"				// how many elements to go?
			
			"vsetvli		t3, zero, e64, m1\n"	// set to maximum VVL
			"vmv.v.x		v16, zero\n"			// splat 0 over v16 (init)
			
		"loop:\n"
			"vsetvli		t3, t2, e64, m1\n"
			"vle.v			v0, (t1)\n"				// unit stride load of the indices
			"vsll.vi		v0, v0, 3\n"			// the index for vlxe expects the indices given as a byte offset!
													//						 	     3      2      1      0
													// Example: data (uint16_t):  0x1020 0x3040 0x5060 0x7080
													//			   index vector:  0x01 0x02							in v0
													//	vlxe.v v1, (data), v0
													
													// Result in v1: 0x6070 and 0x5060 since the index is byte-wise, not element wise
													// To load the uint16_t 0x5060 and 0x3040, the indices have to be shifted by 1 (or multiplied by 2)
													// first. Since we work with 64 bit values here, we have to shift by 3 (or multiply by 8)
			
			"vlxe.v			v8, (%1), v0\n"			// indexed load of the matrix values

//			"vslidedown.vi	v8, v8, 1\n"			// DEBUG: If you want to extract a value in the vector register, you can use this.
//			"vmv.x.s		%0, v8\n"
			
			//"vfredsum.vs	v16, v8, v16\n"			// v16[0] += v8[*]
			"vredsum.vs		v16, v8, v16\n"			// v16[0] += v8[*]
			
			"sub			t2, t2, t3\n"			// how many elements to go?
			"slli			t3, t3, 3\n"			// multiply the number of completed elements by 8, since each element is 8Bytes in size
			"add			t1, t1, t3\n"			// move the index pointer
			"bnez			t2, loop\n"				// jump to loop, if t2 != 0
			
			//"vfmv.f.s		%0, v16\n"				// extract float from vector reg
			"vmv.x.s		%0, v16\n"				// same as vfmv.f.s, just integer

		
		//-- outputs
		: //"=f"(sum_out)	// %0 (double)
		  "=r"(sum_out)		// %0 (long)
		
		//-- inputs
		: "r"(matrix),		// %1 pointer to the matrix
		  "r"(indices),		// %2 pointer to the vector containing the indices
		  "r"(nElements)	// %3 scalar containing the number of elements
		
		//-- clobbers (used registers)
		: "t1", "t2", "t3"
	);
	
	//printf("Result is %ld\n", sum_out);
}


int thread_entry(int cid, int nc) {

	simfence();
	reduction_sum_example(cid, nc, matrix, indices, DIM);
	simfence();

	exit(0);
}
