#/bin/bash!
threads=$1
if [ -z "$1" ]; then
	threads=`nproc --all`
fi

echo "Using $threads threads to compile Sparta. Overwrite this setting with $0 <threads>."

# Pick specific commit to ensure the compatibility with Coyote code

git clone https://github.com/sparcians/map.git || { cd map;git checkout 533e178;cd ..; } 

cd map/sparta
git checkout 533e178
mkdir release
cd release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j $threads

cd ..
mkdir debug
cd debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j $threads

# Go back to coyote-sim
cd ..
