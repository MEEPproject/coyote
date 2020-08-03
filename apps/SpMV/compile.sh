#/bin/bash!

if [ "$#" -gt 2 ] || [ "$#" -lt 1 ]; then
    echo "Usage: #cores [opt]Mat_dim(power of 2)"
    exit 2
fi

if [ "$#" -eq 2 ]; then
    perl spmv_gendata.pl --size $2 > dataset.h
fi

sed -i '/li a7/c li a7, '$1 ./my_crt.S
sed -i '/(\b|[^ABC])n =/c n = '$2';' ./spmv.c

systemctl is-active --quiet docker || sudo service docker start #If docker inactive, activate

sudo docker run -v$PWD/..:/tmp --rm epi_compiler_docker /bin/bash -c "cd /tmp/SpMV; /llvm-EPI-development-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-vectorize -march=rv64gc ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld spmv.c -o spmv"

#riscv64-unknown-elf-gcc -I../common/env -I../common -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -march=rv64gcv ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -lm -lgcc -T ../common/test.ld spmv.c -o spmv
