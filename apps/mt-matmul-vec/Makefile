EPI_TOOLCHAIN=/apps/llvm-toolchain/EPI-0.7/development
CC=${EPI_TOOLCHAIN}/bin/clang
CFLAGS=-mepi -O2 -fno-vectorize  --target=riscv64-unknown-elf -march=rv64g -I/usr/include

LDFLAGS = -lm

.PHONY: all all-linux 

all: all-linux 

all-linux: matmul.linux

matmul.linux: matmul.linux.o
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

matmul.linux.o: matmul.c
	$(CC) $(CFLAGS)  -c -o $@ $<

.PHONY: clean
clean:
	rm -f *.o
	rm -f matmul.linux
