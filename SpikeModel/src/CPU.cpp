// <CPU.cpp> -*- C++ -*-


#include "CPU.hpp"
#include "sparta/utils/SpartaAssert.hpp"

//! \brief Name of this resource. Required by sparta::UnitFactory
constexpr char spike_model::CPU::name[];

//! \brief Constructor of this CPU Unit
spike_model::CPU::CPU(sparta::TreeNode* node, const spike_model::CPU::CPUParameterSet* params) :
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
    varch_(params->varch)
{
    sparta_assert(num_cores_ % num_tiles_ == 0, "The number of tiles must be a divider of the number of cores");
    sparta_assert(num_threads_per_core_ <= num_cores_, "The number of threads per core must be lower or equal to the number of cores");
}

//! \brief Destructor of this CPU Unit
spike_model::CPU::~CPU() = default;
