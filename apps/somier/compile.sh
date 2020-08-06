#/bin/bash!

if [ "$#" -ne 3 ] && [ "$#" -ne 1 ]; then
    echo "Usage: #cores [opt]N [opt]ntsteps"
    exit 2
fi

if [ "$#" -eq 3 ]; then
    perl somier_gendata.pl --size $2 > dataset.h
    sed -i '/ntsteps =/c    ntsteps = '$3';' ./main.c
fi

sed -i '/li a7/c li a7, '$1 ./my_crt.S

systemctl is-active --quiet docker || sudo service docker start #If docker inactive, activate

sudo docker run -v$PWD/..:/tmp --rm epi_compiler_docker /bin/bash -c "cd /tmp/somier; /llvm-EPI-release-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-vectorize -march=rv64gc ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld intrinsics/somier_intr.c main.c intrinsics/forces_prevec.c somier_utils.c omp/somier.c -o somier"

#riscv64-unknown-elf-gcc -I../common/env -I../common -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -march=rv64gcv ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -lm -lgcc -T ../common/test.ld spmv.c -o spmv
