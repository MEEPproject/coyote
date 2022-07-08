#/bin/bash!

threads=$1
if [ -z "$1" ]; then
	threads="1"
fi

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j $threads
