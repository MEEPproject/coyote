import sys

def init_vector(n, value):
    vector_str="{\n\t"
    c=0
    for i in range(n):
            vector_str+=str(value)+", "
            if(c==19):
                vector_str+="\n\t"
                c=0
            else:
                c=c+1
                
    print(vector_str[0:-2]+"\n};\n")

N=sys.argv[1]
A=1.0
B=2.0

print("\n#ifndef __DATASET_H")
print("#define __DATASET_H\n")

print("#define A "+str(A))
print("#define B "+str(B))
print("#define N "+N+"\n")

print("typedef double data_t;")
print("static data_t dx[N] = ")
init_vector(int(N),A)

print("static data_t dy[N] = ")
init_vector(int(N),B)

print("#endif //__DATASET_H")

