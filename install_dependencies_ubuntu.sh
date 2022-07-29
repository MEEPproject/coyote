#!/bin/bash
# 
# Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
#                Supercomputación
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the LICENSE file in the root directory of the project for the
# specific language governing permissions and limitations under the
# License.
# 

threads=$1
if [ -z "$1" ]; then
    threads=`nproc --all`
fi

echo "Using $threads threads to compile the riscv tool chain. Overwrite this setting with $0 <threads>."

#sparta
sudo apt-get install --yes cmake git g++ libboost-all-dev doxygen libsqlite3-dev libyaml-cpp-dev rapidjson-dev libhdf5-dev

#spike
sudo apt-get install --yes device-tree-compiler
sudo apt-get install --yes autoconf automake autotools-dev curl python3 libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev


# Define environment variables for future uses
grep -qxF 'export RISCV=/opt/riscv' ~/.bashrc || echo 'export RISCV=/opt/riscv' >> ~/.bashrc
grep -qxF 'export PATH=$PATH:/opt/riscv/bin' ~/.bashrc || echo 'export PATH=$PATH:/opt/riscv/bin' >> ~/.bashrc

# Define environment variables for current execution
if [ -z "$RISCV" ];
then
    export RISCV=/opt/riscv
    export PATH=$PATH:/opt/riscv/bin
fi

# Download and install riscv-gnu-toolchain
git clone --recursive https://github.com/riscv/riscv-gnu-toolchain -b rvv-0.8.x
cd riscv-gnu-toolchain
git pull
git checkout rvv-0.8.x --recurse-submodules
./configure --prefix=$RISCV
sudo make -j $threads
cd ..

# Download and install riscv-pk
git clone https://github.com/riscv/riscv-pk.git
cd riscv-pk
git pull
mkdir build
cd build
../configure --prefix=$RISCV --host=riscv64-unknown-elf
make -j $threads
sudo make install

# Go back to coyote-sim
cd ../..

# Execute bash to reload variables for future scripts
exec bash
