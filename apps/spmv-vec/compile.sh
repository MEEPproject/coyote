#/bin/bash!

# Default problem size
size=128

if [ "$#" -gt 2 ] || [ "$#" -lt 1 ]; then
    echo "Usage: #cores [opt]Mat_dim(power of 2)"
    exit 2
fi

if [ "$#" -eq 2 ]; then
    perl spmv_gendata.pl --size $2 > dataset.h
else
    if [ ! -f dataset.h ]
    then
        perl spmv_gendata.pl --size $size > dataset.h
    fi
fi

sed -i '/li a7/c li a7, '$1 ./my_crt.S

# Is docker down?
sudo service docker status
if [ $? != 0 ]; then
    sudo service docker start #If docker inactive, activate
    if [ $? = 0 ]; then
	    sleep 1
        sudo docker run -v$PWD/..:/tmp --rm epi_compiler_docker /bin/bash -c "cd /tmp/spmv-vec; /llvm-EPI-development-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-vectorize -march=rv64gc ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld spmv.c -o spmv"
    else # docker is not able to start or not exists -> run epi_compiler directly
        ../../epi_compiler/llvm-EPI-release-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-vectorize -march=rv64gc ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld spmv.c -o spmv
    fi
else
	sudo docker run -v$PWD/..:/tmp --rm epi_compiler_docker /bin/bash -c "cd /tmp/spmv-vec; /llvm-EPI-development-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-vectorize -march=rv64gc ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld spmv.c -o spmv"
fi
