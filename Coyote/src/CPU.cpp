// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 

// <CPU.cpp> -*- C++ -*-


#include "CPU.hpp"
#include "sparta/utils/SpartaAssert.hpp"

//! \brief Name of this resource. Required by sparta::UnitFactory
constexpr char coyote::CPU::name[];

//! \brief Constructor of this CPU Unit
coyote::CPU::CPU(sparta::TreeNode* node, const coyote::CPU::CPUParameterSet* params) :
    sparta::Unit{node},
    frequency_ghz_{params->frequency_ghz},
    num_cores_(params->num_cores),
    num_threads_per_core_(params->num_threads_per_core),
    thread_switch_latency_(params->thread_switch_latency),
    num_tiles_(params->num_tiles),
    num_memory_cpus_(params->num_memory_cpus),
    num_memory_controllers_(params->num_memory_controllers),
    isa_(params->isa),
    icache_config_(params->icache_config),
    dcache_config_(params->dcache_config),
    varch_(params->varch),
    x_size_(params->x_size),
    y_size_(params->y_size),
    mcpus_indices_(params->mcpus_indices)
{
    sparta_assert(num_cores_ % num_tiles_ == 0, "The number of tiles must be a divider of the number of cores");
    sparta_assert(num_threads_per_core_ <= num_cores_, "The number of threads per core must be lower or equal to the number of cores");
    sparta_assert(x_size_ * y_size_ == num_memory_cpus_ + num_tiles_, 
        "The NoC mesh size must be equal to the number of elements to connect:" << 
        "\n X: " << x_size_ <<
        "\n Y: " << y_size_ <<
        "\n PEs: " << num_memory_cpus_ + num_tiles_);
    sparta_assert(params->mcpus_indices.isVector(), "The top.arch.params.mcpus_indices must be a vector");
    sparta_assert(params->mcpus_indices.getNumValues() == num_memory_cpus_, 
        "The number of elements in mcpus_indices must be equal to the number of MCPUs");
}

//! \brief Destructor of this CPU Unit
coyote::CPU::~CPU() = default;
