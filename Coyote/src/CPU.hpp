// <CPU.h> -*- C++ -*-


#ifndef __CPU_H__
#define __CPU_H__

#include <string>

#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/simulation/ParameterSet.hpp"

namespace coyote{

/**
 * @file  CPU.h
 * @brief CPU Unit acts as a logical unit containing multiple cores
 *
 * CPU unit will
 * 1. Attach itself to the root Simulation Device node
 * 2. Use its factory to create multiple cores
 * 3. Use sub-factories within its factory to create microarchitecture units
 */
class CPU : public sparta::Unit{
public:
    
    /*!
     * \class spike:model::CPU
     * \brief Representation of the overall modelled architecture
     *
     */

    //! \brief Parameters for CPU model
    class CPUParameterSet : public sparta::ParameterSet{
    public:
        CPUParameterSet(sparta::TreeNode* n) : sparta::ParameterSet(n){}
        // Dummy configuration parameters and environment variables that affect CPU utilization
        PARAMETER(double, frequency_ghz, 1.2, "CPU Clock frequency")
        PARAMETER(uint16_t, num_cores, 1, "The number of cores to simulate")
        PARAMETER(uint16_t, num_threads_per_core, 1, "The number of threads per core to simulate")
        PARAMETER(uint16_t, thread_switch_latency, 0, "Number of cycles required to make the thread runnable")
        PARAMETER(uint16_t, num_tiles, 1, "The number of tiles to simulate")
        PARAMETER(uint16_t, num_memory_cpus, 1, "The number of memory cpus")
        PARAMETER(uint16_t, num_memory_controllers, 1, "The number of memory controllers")
        PARAMETER(std::string, isa, "RV64IMAFDCV", "The RISC-V isa version to use")
        PARAMETER(std::string, icache_config, "64:8:64", "The icache configuration")
        PARAMETER(std::string, dcache_config, "64:8:64", "The dcache configuration")
        PARAMETER(std::string, varch, "v128:e64:s128", "The varch to simulate")
        PARAMETER(uint16_t, x_size, 2, "The size of X dimension")
        PARAMETER(uint16_t, y_size, 1, "The size of Y dimension")
        PARAMETER(std::vector<uint16_t>, mcpus_indices, {0}, "The indices of MCPUs in the network ordered by MCPU")
    };

    //! \brief Name of this resource. Required by sparta::UnitFactory
    static constexpr char name[] = "arch";

    /**
     * @brief Constructor for CPU
     *
     * @param node The node that represents (has a pointer to) the CPU
     * @param p The CPU's parameter set
     */
    CPU(sparta::TreeNode* node, const CPUParameterSet* params);

    //! \brief Destructor of the CPU Unit
    ~CPU();
private:

    //! \brief Internal configuration units of this processor
    double frequency_ghz_;
    uint16_t num_cores_;
    uint16_t num_threads_per_core_;
    uint16_t thread_switch_latency_;
    uint16_t num_tiles_;
    uint16_t num_memory_cpus_;
    uint16_t num_memory_controllers_;
    std::string isa_;
    std::string icache_config_;
    std::string dcache_config_;
    std::string varch_;
    uint16_t                            x_size_;            //! The size of X dimension
    uint16_t                            y_size_;            //! The size of Y dimension
    std::vector<uint16_t>               mcpus_indices_;     //! The indices of MCPUs in the network ordered by MCPU

}; // class CPU
}  // namespace coyote
#endif
