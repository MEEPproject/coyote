import numpy as np
from scipy.sparse import csr_matrix
import sys

def init_vector(n, value):
    vector_str="\t"
    c=0
    for i in range(n):
            vector_str+=str(value)+", "
            if(c==19):
                vector_str+="\n\t"
                c=0
            else:
                c=c+1
                
    print(vector_str[0:-2])

filename=sys.argv[1]
res = []
with open(filename) as fp:
    for i, line in enumerate(fp):
        if i == 2:
           res = [int(i) for i in line.split() if i.isdigit()]
           break


#res[0] is number of columns, res[1] is number of rows, res[2] is number of non-zero values
col, row, data = np.loadtxt(filename, unpack=True, skiprows=3)
row = row - 1
col = col - 1
matrix = csr_matrix((data, (row, col)), shape=(res[1], res[0]))

print("\n#ifndef __DATASET_H")
print("#define __DATASET_H\n")

print("#define N_ROWS "+str(res[1]))
print("#define N_NON_ZERO "+str(matrix.nnz)+"\n")

print("static long ia[N_ROWS+1] __attribute__((aligned (1024))) = \n{")
print("\t"+str(matrix.indptr.tolist())[1:-1])
print("};\n")

print("static long ja[N_NON_ZERO] __attribute__((aligned (1024))) = \n{")
print("\t"+str(matrix.indices.tolist())[1:-1])
print("};\n")

print("static double a[N_NON_ZERO] __attribute__((aligned (1024))) = \n{")
print("\t"+str(matrix.data.tolist())[1:-1])
print("};\n")
print("static double x[N_ROWS] __attribute__((aligned (1024))) = \n{")
init_vector(res[1],13)
print("};\n")

print("#endif //__DATASET_H")
