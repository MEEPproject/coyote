// <CPUFactories.h> -*- C++ -*-

#ifndef __CPU_FACTORIES_H__
#define __CPU_FACTORIES_H__

#include "sparta/simulation/ResourceFactory.hpp"
#include "Tile.hpp"
#include "CacheBank.hpp"
#include "NoC/FunctionalNoC.hpp"
#include "NoC/SimpleNoC.hpp"
#include "MemoryCPUWrapper.hpp"
#include "MemoryController.hpp"
#include "MemoryBank.hpp"

namespace spike_model{

struct CPUFactories{
    /*
     * \struct spike_model::CPUFactories
     * \brief CPUFactories will act as the place which contains all the required factories to build sub-units of the CPU
     */

    //! \brief Resource Factory to build a CacheBank Unit
    sparta::ResourceFactory<spike_model::CacheBank,
                          spike_model::CacheBank::CacheBankParameterSet> cache_bank_rf;

    //! \brief Resouce Factory to build a functional NoC Unit
    sparta::ResourceFactory<spike_model::FunctionalNoC,
                            spike_model::FunctionalNoC::FunctionalNoCParameterSet> functional_noc_rf;

    //! \brief Resouce Factory to build a simple NoC Unit
    sparta::ResourceFactory<spike_model::SimpleNoC,
                            spike_model::SimpleNoC::SimpleNoCParameterSet> simple_noc_rf;

    //! \brief Resource Factory to build a Tile Unit
    sparta::ResourceFactory<spike_model::Tile,
                          spike_model::Tile::TileParameterSet> tile_rf;

    //! \brief Resource Factory to build a Memory CPU
    sparta::ResourceFactory<spike_model::MemoryCPUWrapper,
                          spike_model::MemoryCPUWrapper::MemoryCPUWrapperParameterSet> memory_cpu_rf;

    //! \brief Resource Factory to build a MemoryController Unit
    sparta::ResourceFactory<spike_model::MemoryController,
                          spike_model::MemoryController::MemoryControllerParameterSet> memory_controller_rf;

    //! \brief Resource Factory to build a MemoryBank Unit
    sparta::ResourceFactory<spike_model::MemoryBank,
                          spike_model::MemoryBank::MemoryBankParameterSet> memory_bank_rf;

}; // struct CPUFactories

}  // namespace spike_model

#endif // __CPU_FACTORIES_H__
