#/bin/bash!

#sparta
sudo apt-get install cmake git g++ libboost-all-dev doxygen libsqlite3-dev libyaml-cpp-dev rapidjson-dev libhdf5-dev

#spike
sudo apt-get install device-tree-compiler
sudo apt-get install autoconf automake autotools-dev curl python3 libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev


ex -sc '$a
export RISCV=/opt/riscv
.
$-,$!uniq
x' ~/.bashrc

git clone --recursive https://github.com/riscv/riscv-gnu-toolchain 
cd riscv-gnu-toolchain
git checkout rvv-0.8.x
./configure --prefix=$RISCV
sudo make
cd ..

export PATH=$PATH:/opt/riscv/bin

git clone https://github.com/riscv/riscv-pk.git
cd riscv-pk
mkdir build
cd build
../configure --prefix=$RISCV --host=riscv64-unknown-elf
make
sudo make install
