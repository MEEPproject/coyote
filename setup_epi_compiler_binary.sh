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

# EPI-compiler links to install
epi_7_url=https://admin.hca.bsc.es/epi/ftp/llvm-EPI-0.7-release-toolchain-cross-2020-03-09-1237.tar.bz2
epi_url=https://admin.hca.bsc.es/epi/ftp/llvm-EPI-release-toolchain-cross-2020-05-12-1155.tar.bz2

# aux variables
epi_7_file="$(cut -d'/' -f6 <<< "$epi_7_url")"
epi_file="$(cut -d'/' -f6 <<< "$epi_url")"

# Setup epi-compiler in binary mode

# Dependencies
sudo apt-get update
sudo apt-get install --yes wget gcc-multilib g++-multilib

# Remove previous installation
if [ -d "./epi_compiler" ]; then
    rm -rf ./epi_compiler
fi

# Install folder
mkdir epi_compiler

# Download files
cd epi_compiler
wget $epi_7_url && tar -xjf $epi_7_file
wget $epi_url && tar -xjf $epi_file
rm $epi_7_file
rm $epi_file
cd ..
