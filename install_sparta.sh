#/bin/bash!

git clone https://github.com/sparcians/map.git

cd map/sparta
mkdir release
cd release
cmake .. -DCMAKE_BUILD_TYPE=Release
make

cd ..
mkdir debug
cd debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
