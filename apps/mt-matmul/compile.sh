#!/bin/bash

# Default problem size
size=128

if [ "$#" -gt 2 ] || [ "$#" -lt 1 ]; then
    echo "Usage: #cores [opt]Mat_dim(power of 2)"
    exit 2
fi

if [ "$#" -eq 2 ]; then
    min_size=$((4*$1)); # 4 is the BLOCK_SIZE defined in matmul.c
    if [ $2 -lt $min_size ]
    then
        echo "ERROR: The Mat_dim must be at least 4 * #cores"
        exit 2
    else
        perl matmul_gendata.pl --size $2 > dataset.h
    fi
else
    if [ ! -f dataset.h ]
    then
        perl matmul_gendata.pl --size $size > dataset.h
    fi
fi

sed -i '/li a7/c li a7, '$1 ./my_crt.S


#-- march parameter settings
#   https://www.sifive.com/blog/all-aboard-part-1-compiler-args
#
#    M: Integer Multiplication and Division
#    A: Atomic Instructions
#    F: Single-Precision Floating-Point
#    D: Double-Precision Floating-Point
#    C: Compressed Instructions


#riscv64-unknown-elf-gcc -I../common/env -I../common -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -march=rv64gcv ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -lm -lgcc -T ../common/test.ld matmul.c mt-matmul.c -o matmul
riscv64-unknown-elf-gcc -I../common/env -I../common -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -march=rv64imafdc ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -lm -lgcc -T ../common/test.ld matmul.c mt-matmul.c -o matmul
