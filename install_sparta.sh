#/bin/bash!
threads=$1
if [ -z "$1" ]; then
	threads="1"
fi

git clone https://github.com/sparcians/map.git || cd map;git pull;cd .. 

cd map/sparta
git pull
mkdir release
cd release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j $threads

cd ..
mkdir debug
cd debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j $threads
