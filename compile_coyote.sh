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

echo "Using $threads threads to compile Spike, Booksim, and Coyote. Overwrite this setting with $0 <threads>."

# Spike

cd riscv-isa-sim
mkdir -p build
cd build
../configure --prefix=$RISCV
if [ $? -ne 0 ]; then
	cd ../../
	exit 1
fi
make -j $threads
if [ $? -ne 0 ]; then
	cd ../../
	exit 1
fi
cd ../../

# BookSim

cd booksim/booksim2
make lib_static -j $threads
if [ $? -ne 0 ]; then
	cd ../../
	exit 1
fi
cd ../../

# Coyote

cd Coyote
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j $threads
