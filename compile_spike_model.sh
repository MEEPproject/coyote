#/bin/bash!
threads=$1
if [ -z "$1" ]; then
	threads="1"
fi

#Spike

cd riscv-isa-sim
mkdir -p build
cd build
../configure --prefix=$RISCV
if [ $? -ne 0 ]; then
	exit
fi

make -j $threads
if [ $? -ne 0 ]; then
	exit
fi

cd ../../

# BookSim

cd booksim/booksim2
make lib_static -j $threads
if [ $? -ne 0 ]; then
	exit
fi
cd ../../

#SpikeModel

cd SpikeModel
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j $threads


