// <CPUFactories.h> -*- C++ -*-


#ifndef __CPU_FACTORIES_H__
#define __CPU_FACTORIES_H__

#include "sparta/simulation/ResourceFactory.hpp"
#include "Tile.hpp"
#include "MemoryController.hpp"
#include "CacheBank.hpp"
#include "NoC.hpp"
#include "MemoryBank.hpp"

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

//    sparta::ResourceFactory<spike_model::Core,
//                          spike_model::Core::CoreParameterSet> core_rf;
    
    sparta::ResourceFactory<spike_model::CacheBank,
                          spike_model::CacheBank::CacheBankParameterSet> cache_bank_rf;
    
    sparta::ResourceFactory<spike_model::NoC,
                          spike_model::NoC::NoCParameterSet> noc_rf;
    
    sparta::ResourceFactory<spike_model::Tile,
                          spike_model::Tile::TileParameterSet> tile_rf;

    sparta::ResourceFactory<spike_model::MemoryController,
                          spike_model::MemoryController::MemoryControllerParameterSet> memory_controller_rf;
    
    sparta::ResourceFactory<spike_model::MemoryBank,
                          spike_model::MemoryBank::MemoryBankParameterSet> memory_bank_rf;
}; // struct CPUFactories
}  // namespace spike_model
#endif
