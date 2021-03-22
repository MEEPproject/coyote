#/bin/bash!

# Default problem size
n=100
steps=10

if [ "$#" -ne 3 ] && [ "$#" -ne 1 ]; then
    echo "Usage: #cores [opt]N [opt]ntsteps"
    exit 2
fi

if [ "$#" -eq 3 ]; then
    perl somier_gendata.pl --size $2 > dataset.h
    sed -i '/ntsteps =/c    ntsteps = '$3';' ./main.c
else
    if [ ! -f dataset.h ]
    then
        perl somier_gendata.pl --size $n > dataset.h
        sed -i '/ntsteps =/c    ntsteps = '$steps';' ./main.c
    fi
fi

sed -i '/li a7/c li a7, '$1 ./my_crt.S

# Is docker down?
sudo service docker status
if [ $? != 0 ]; then
    sudo service docker start #If docker inactive, activate
    if [ $? = 0 ]; then
	    sleep 1
        sudo docker run -v$PWD/..:/tmp --rm epi_compiler_docker /bin/bash -c "cd /tmp/somier; /llvm-EPI-release-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-vectorize -march=rv64gc ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld intrinsics/somier_intr.c main.c intrinsics/forces_prevec.c somier_utils.c omp/somier.c -o somier"
    else # docker is not able to start or not exists -> run epi_compiler directly
        ../../epi_compiler/llvm-EPI-release-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-vectorize -march=rv64gc ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld intrinsics/somier_intr.c main.c intrinsics/forces_prevec.c somier_utils.c omp/somier.c -o somier
    fi
else
	sudo docker run -v$PWD/..:/tmp --rm epi_compiler_docker /bin/bash -c "cd /tmp/somier; /llvm-EPI-release-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-vectorize -march=rv64gc ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld intrinsics/somier_intr.c main.c intrinsics/forces_prevec.c somier_utils.c omp/somier.c -o somier"
fi
