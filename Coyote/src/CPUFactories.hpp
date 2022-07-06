// <CPUFactories.h> -*- C++ -*-

#ifndef __CPU_FACTORIES_H__
#define __CPU_FACTORIES_H__

#include "sparta/simulation/ResourceFactory.hpp"
#include "Tile.hpp"
#include "L2CacheBank.hpp"
#include "L3CacheBank.hpp"
#include "NoC/FunctionalNoC.hpp"
#include "NoC/SimpleNoC.hpp"
#include "NoC/DetailedNoC.hpp"
#include "MemoryTile/MemoryCPUWrapper.hpp"
#include "MemoryTile/MemoryController.hpp"
#include "MemoryTile/MemoryBank.hpp"

namespace coyote{

struct CPUFactories{
    /*
     * \struct coyote::CPUFactories
     * \brief CPUFactories will act as the place which contains all the required factories to build sub-units of the CPU
     */

    //! \brief Resource Factory to build a CacheBank Unit
    sparta::ResourceFactory<coyote::L2CacheBank,
                          coyote::L2CacheBank::L2CacheBankParameterSet> cache_bank_rf;

    //! \brief Resource Factory to build a functional NoC Unit
    sparta::ResourceFactory<coyote::FunctionalNoC,
                            coyote::FunctionalNoC::FunctionalNoCParameterSet> functional_noc_rf;

    //! \brief Resource Factory to build a simple NoC Unit
    sparta::ResourceFactory<coyote::SimpleNoC,
                            coyote::SimpleNoC::SimpleNoCParameterSet> simple_noc_rf;

    //! \brief Resource Factory to build a detailed NoC Unit
    sparta::ResourceFactory<coyote::DetailedNoC,
                            coyote::DetailedNoC::DetailedNoCParameterSet> detailed_noc_rf;

    //! \brief Resource Factory to build a Tile Unit
    sparta::ResourceFactory<coyote::Tile,
                          coyote::Tile::TileParameterSet> tile_rf;

    //! \brief Resource Factory to build a Memory CPU
    sparta::ResourceFactory<coyote::MemoryCPUWrapper,
                          coyote::MemoryCPUWrapper::MemoryCPUWrapperParameterSet> memory_cpu_rf;

    //! \brief Resource Factory to build a MemoryController Unit
    sparta::ResourceFactory<coyote::MemoryController,
                          coyote::MemoryController::MemoryControllerParameterSet> memory_controller_rf;

    //! \brief Resource Factory to build a MemoryBank Unit
    sparta::ResourceFactory<coyote::MemoryBank,
                          coyote::MemoryBank::MemoryBankParameterSet> memory_bank_rf;

    //! \brief Resource Factory to build a L3 Cache Bank Unit
    sparta::ResourceFactory<coyote::L3CacheBank,
                          coyote::L3CacheBank::L3CacheBankParameterSet> cache_bank_llc_rf;
                          
    //! \brief Resource Factory to build a Arbiter Unit
    sparta::ResourceFactory<coyote::Arbiter,
                          coyote::Arbiter::ArbiterParameterSet> arbiter_rf;
}; // struct CPUFactories

}  // namespace coyote

#endif // __CPU_FACTORIES_H__
