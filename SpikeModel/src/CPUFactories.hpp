// <CPUFactories.h> -*- C++ -*-


#ifndef __CPU_FACTORIES_H__
#define __CPU_FACTORIES_H__

#include "sparta/simulation/ResourceFactory.hpp"
#include "Core.hpp"
#include "L2Cache.hpp"
#include "NoC.hpp"
#include "spike_wrapper.h"

namespace spike_model{

/**
 * @file  CPUFactories.h
 * @brief CPUFactories will act as the place which contains all the
 *        required factories to build sub-units of the CPU.
 *
 * CPUFactories unit will
 * 1. Contain resource factories to build each core of the CPU
 * 2. Contain resource factories to build microarchitectural units in each core
 */
struct CPUFactories{

    //! \brief Resouce Factory to build a Core Unit

    sparta::ResourceFactory<spike_model::Core,
                          spike_model::Core::CoreParameterSet> core_rf;
    
    sparta::ResourceFactory<spike_model::L2Cache,
                          spike_model::L2Cache::L2CacheParameterSet> l2_rf;
    
    sparta::ResourceFactory<spike_model::NoC,
                          spike_model::NoC::NoCParameterSet> noc_rf;

    sparta::ResourceFactory<spike_model::SpikeWrapper,
                          spike_model::SpikeWrapper::SpikeWrapperParameterSet> spike_rf;

}; // struct CPUFactories
}  // namespace spike_model
#endif
