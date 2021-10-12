#/bin/bash!

# Default problem size
size=128

if [ ! -f dataset.h ]; then
	perl gendata.pl --size $size > dataset.h
fi

cores=1

sed -i '/li a7/c li a7, '${cores} ./my_crt.S

/llvm-EPI-release-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-vectorize -march=rv64gc ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld test.c -o test
