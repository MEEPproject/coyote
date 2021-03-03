// <CPUTopology.h> -*- C++ -*-


#ifndef __CPU_TOPOLOGY_H__
#define __CPU_TOPOLOGY_H__

#include <memory>

#include "CPU.hpp"
#include "CPUFactories.hpp"
#include "sparta/simulation/ResourceFactory.hpp"
#include "sparta/simulation/RootTreeNode.hpp"
#include "sparta/simulation/ResourceTreeNode.hpp"
#include "Logger.hpp"

namespace spike_model{

class CPUTopology{
public:

    /**
     * \class  spike_model::CPUTopology
     * \brief CPUTopology will act as the place where a user-defined topology
     *        is actually written. This class has structures for holding the
     *        required tree nodes and details about its parents nodes, names,
     *        groups, ids and whether it should be a private node or not.
     *
     * CPUTopology unit will
     * 1. Contain the nuts and bolts needed by the user to generate an actual topology
     * 2. Contain unit structures and port structures to build and bind
     * 3. Allow deriving classes to define a topology
     */

    //! \brief Structure to represent a resource unit in device tree
    struct UnitInfo{

        //! ResourceTreeNode name
        std::string name;

        //! ResourceTreeNode parent name
        std::string parent_name;

        //! ResourceTreeNode human-readable name
        std::string human_name;

        //! TreeNode group name required for multiple execution units
        std::string group_name;

        //! TreeNode group id required for multiple execution units
        uint32_t group_id;

        //! Factory required to create this particular resource
        sparta::ResourceFactoryBase* factory;

        //! Flag to tell whether this node should be private to its parent
        bool is_private_subtree;

        /**
         * @brief Constructor for UnitInfo
         */
        UnitInfo(const std::string& name,
                 const std::string& parent_name,
                 const std::string& human_name,
                 const std::string&  group_name,
                 const uint32_t group_id,
                 sparta::ResourceFactoryBase* factory,
                 bool is_private_subtree = false) :
            name{name},
            parent_name{parent_name},
            human_name{human_name},
            group_name{group_name},
            group_id{group_id},
            factory{factory},
            is_private_subtree{is_private_subtree}{}
    };

    //! \brief Structure to represent a port binding between units in device tree
    struct PortConnectionInfo{

        //! Out port name of unit_1
        std::string output_port_name;

        //! In port name of next unit, unit_2
        std::string input_port_name;

        /**
         * @brief Constructor for PortConnectionInfo
         */
        PortConnectionInfo(const std::string& output_port_name,
                           const std::string& input_port_name) :
            output_port_name{output_port_name},
            input_port_name{input_port_name}{}
    };

    /**
     * @brief Constructor for CPUTopology
     */
    CPUTopology() : factories{new CPUFactories()}{}

    ~CPUTopology()
    {
        logger.close();
    }

    /**
     * @brief Set the number of cores in this processor
     */
    auto setNumTiles(const uint32_t num_of_tiles) -> void{
        num_tiles = num_of_tiles;
    }

    /**
     * @brief Set the name for this topoplogy
     */
    auto setName(const std::string& topology) -> void{
        topology_name = topology;
    }

    /**
     * @brief Set the number of memory CPUs
     */
    auto setNumMemoryCPUs(const uint32_t num_of_memory_cpus) -> void{
        num_memory_cpus = num_of_memory_cpus;
    }

    /**
     * @brief Set the number of memory controllers
     */
    auto setNumMemoryControllers(const uint32_t num_of_memory_controllers) -> void{
        num_memory_controllers = num_of_memory_controllers;
    }

    /**
     * @brief Set the number of memory banks handled per memory controller
     */
    auto setNumMemoryBanksPerMemoryController(const uint32_t num_of_memory_banks) -> void{
        num_memory_banks = num_of_memory_banks;
    }

    /**
     * @brief Set the number of L2 banks in this processor
     */
    auto setNumL2BanksPerTile(const uint32_t num_of_l2_banks) -> void{
        num_banks_per_tile = num_of_l2_banks;
    }

    /**
     * @brief Set whether to trace or not
     */
    auto setTrace(const bool t) -> void{
        trace = t;
    }

    /**
     * @brief Static method to allocate memory for topology
     */
    static auto allocateTopology(const std::string& topology) -> CPUTopology*;

    //! Public members used by CPUFactory to build and bind tree
    uint32_t num_tiles;
    uint32_t num_banks_per_tile;
    uint32_t num_memory_cpus;
    uint32_t num_memory_controllers;
    uint32_t num_memory_banks;
    bool trace;

    spike_model::Logger logger;

    std::unique_ptr<CPUFactories> factories;

    std::string topology_name;
    std::vector<UnitInfo> units;
    std::vector<UnitInfo> shared_units;
    std::vector<PortConnectionInfo> port_connections;

}; // class CPUTopology


class CoreTopology_4 : public CPUTopology{
public:

    /**
     * @brief Constructor for CPUTopology
     */
    CoreTopology_4();
}; // class CoreTopology_3
}  // namespace spike_model
#endif
