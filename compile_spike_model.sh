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
make -j $threads
sudo make install
cd ../../

#SpikeModel

cd SpikeModel
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j $threads


