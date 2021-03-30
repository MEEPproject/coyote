// <CPUTopology.cpp> -*- C++ -*-

#include "CPUTopology.hpp"
#include "sparta/utils/SpartaException.hpp"

/*
 * @brief Constructor for CPUTopology_1
 */

spike_model::CoreTopology_4::CoreTopology_4(){

//! Instantiating units of this topology
    units = {
            {
                "tile*",
                "cpu",
                "Tile *",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->tile_rf
            },
            {
                "l2_bank$",
                "cpu.tile*",
                "L2 Cache Bank $",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->cache_bank_rf
            },
            {
                "memory_cpu&",
                "cpu",
                "Memory CPU &",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->memory_cpu_rf
            },
            {
                "memory_controller&",
                "cpu",
                "Memory Controller &",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->memory_controller_rf
            },
            {
                "memory_bank!",
                "cpu.memory_controller&",
                "Memory Bank !",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->memory_bank_rf
            },
    };
    //! Instantiating ports of this topology
    port_connections = {
        {
            "cpu.tile*.ports.out_l2_bank$_req",
            "cpu.tile*.l2_bank$.ports.in_tile_req"
        },
        {
            "cpu.tile*.ports.out_l2_bank$_ack",
            "cpu.tile*.l2_bank$.ports.in_tile_ack"
        },
        {
            "cpu.tile*.ports.in_l2_bank$_ack",
            "cpu.tile*.l2_bank$.ports.out_tile_ack"
        },
        {
            "cpu.tile*.l2_bank$.ports.out_tile_req",
            "cpu.tile*.ports.in_l2_bank$_req"
        },
        {
            "cpu.noc.ports.out_tile*",
            "cpu.tile*.ports.in_noc"
        },
        {
            "cpu.noc.ports.in_tile*",
            "cpu.tile*.ports.out_noc"
        },
        {
            "cpu.noc.ports.in_memory_cpu&",
            "cpu.memory_cpu&.ports.out_noc"
        },
        {
            "cpu.noc.ports.out_memory_cpu&",
            "cpu.memory_cpu&.ports.in_noc"
        },
        {
            "cpu.memory_controller&.ports.in_mcpu",
            "cpu.memory_cpu&.ports.out_mc"
        },
        {
            "cpu.memory_controller&.ports.out_mcpu",
            "cpu.memory_cpu&.ports.in_mc"
        },
    };
    
    shared_units = {
            {
                "noc",
                "cpu",
                "NoC",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                NULL                                // This is filled in CPUFactory when it is instantiated
            },
    };
}
/*
 * @brief Static method to allocate memory for topology
 */
auto spike_model::CPUTopology::allocateTopology(const std::string& topology) -> spike_model::CPUTopology*{
    CPUTopology* new_topology {nullptr};
    if(topology == "core_topology_4")
    {
        new_topology = new spike_model::CoreTopology_4();
    }
    else{
        throw sparta::SpartaException("This topology in unrecognized.");
    }
    sparta_assert(new_topology);
    return new_topology;
}
