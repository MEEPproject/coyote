
version=$1

if [ "$version"  = "scalar" ]; then
    compiler_args="-Wall --target=riscv64-unknown-elf -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -ffast-math -fno-common -fno-builtin-printf -march=rv64g -O3 -nostdlib -nostartfiles -T ../common/test.ld -o stream_serial ../common/syscalls.c src/my_crt.S  src/stream.c"
elif [ "$version" = "vector" ]; then
    compiler_args="-mepi -Wall --target=riscv64-unknown-elf -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -ffast-math -fno-common -fno-builtin-printf -march=rv64g -DVL_ELEM=256 -DTUNED -O3 -fno-vectorize -nostdlib -nostartfiles -T ../common/test.ld -o stream_vectorial ../common/syscalls.c src/my_crt.S src/stream.c"
else
    echo "Unsupported version!"
    exit 0
fi


# Is docker down?
sudo service docker status
if [ $? != 0 ]; then
    sudo service docker start #If docker inactive, activate
    if [ $? = 0 ]; then
       sleep 1
    sudo docker run -v$PWD/..:/tmp --rm epi_compiler_docker /bin/bash -c "cd /tmp/stream; /llvm-EPI-release-toolchain-cross/bin/clang -mepi $compiler_args"
    else # docker is not able to start or not exists -> run epi_compiler directly
        ../../epi_compilerllvm-EPI-release-toolchain-cross/bin/clang $compiler_args
    fi
else
    sudo docker run -v$PWD/..:/tmp --rm epi_compiler_docker /bin/bash -c "cd /tmp/stream; /llvm-EPI-release-toolchain-cross/bin/clang $compiler_args"
fi
