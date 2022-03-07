#makefile
EPI_VERSION=-0.7
#EPI_VERSION=
COYOTE=$(HOME)/simulation/coyote-sim
LLVM=$(COYOTE)/epi_compiler/llvm-EPI-release-toolchain-cross

all: dirs serial vector

dirs:
	mkdir -p bin
	rm -f bin/*.*

serial:
	${LLVM}/bin/clang -Wall --target=riscv64-unknown-elf -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -ffast-math -fno-common -fno-builtin-printf -march=rv64g -O3 -nostdlib -nostartfiles -T ../common/test.ld -o stream_serial ../common/syscalls.c src/my_crt.S  src/stream.c

vector:
	${LLVM}/bin/clang -mepi -Wall --target=riscv64-unknown-elf -I../common/env -I../common -I/usr/include/ -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -ffast-math -fno-common -fno-builtin-printf -march=rv64g -DVL_ELEM=256 -DTUNED -O3 -fno-vectorize -nostdlib -nostartfiles -T ../common/test.ld -o stream_vectorial ../common/syscalls.c src/my_crt.S src/stream.c

clean:
	rm -f bin/*
