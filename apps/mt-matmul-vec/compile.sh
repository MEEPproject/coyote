#/bin/bash!

if [ "$#" -gt 2 ] || [ "$#" -lt 1 ]; then
    echo "Usage: #cores [opt]Mat_dim(power of 2)"
    exit 2
fi

if [ "$#" -eq 2 ]; then
    perl matmul_gendata.pl --size $2 > dataset.h
fi

sed -i '/li a7/c li a7, '$1 ./my_crt.S

systemctl is-active --quiet docker || sudo service docker start #If docker inactive, activate

sudo docker run -v$PWD/..:/tmp --rm epi_compiler_docker /bin/bash -c "cd /tmp/mt-matmul-vec; /llvm-EPI-release-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -march=rv64g ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld matmul.c -o matmul
"
#~/local/llvm-EPI-0.7-development-toolchain-cross/bin/clang --target=riscv64-unknown-elf -mepi -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -march=rv64g ../common/syscalls.c ./my_crt.S -static -nostdlib -nostartfiles -T ../common/test.ld matmul.c -o matmul
