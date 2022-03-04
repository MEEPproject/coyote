// <CPUTopology.cpp> -*- C++ -*-

#include "CPUTopology.hpp"
#include "sparta/utils/SpartaException.hpp"

/*
 * @brief Constructor for CPUTopology_1
 */

spike_model::TiledTopology::TiledTopology(){

//! Instantiating units of this topology
    units = {
            {
                "tile*",
                "arch",
                "Tile *",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->tile_rf
            },
            {
                "l2_bank$",
                "arch.tile*",
                "L2 Cache Bank $",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->cache_bank_rf
            },
            {
                "memory_cpu&",
                "arch",
                "Memory CPU &",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->memory_cpu_rf
            },
            {
                "memory_controller#",
                "arch",
                "Memory Controller #",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->memory_controller_rf
            },
            {
                "llc^",
                "arch.memory_cpu&",
                "LLC ^",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->cache_bank_llc_rf
            },
            {
                "memory_bank!",
                "arch.memory_controller#",
                "Memory Bank !",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->memory_bank_rf
            },
            {
                "arbiter",
                "arch.tile*",
                "Arbiter @",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->arbiter_rf
            },

    };
    //! Instantiating ports of this topology
    port_connections = {
        {
            "arch.tile*.ports.out_l2_bank$_req",
            "arch.tile*.l2_bank$.ports.in_tile_req"
        },
        {
            "arch.tile*.ports.out_l2_bank$_ack",
            "arch.tile*.l2_bank$.ports.in_tile_ack"
        },
        {
            "arch.tile*.ports.in_l2_bank$_ack",
            "arch.tile*.l2_bank$.ports.out_tile_ack"
        },
        {
            "arch.tile*.l2_bank$.ports.out_tile_req",
            "arch.tile*.ports.in_l2_bank$_req"
        },
        {
            "arch.noc.ports.out_tile*",
            "arch.tile*.ports.in_noc"
        },
        {
            "arch.tile*.arbiter.ports.in_tile",
            "arch.tile*.ports.out_arbiter"
        },
        {
            "arch.noc.ports.in_memory_cpu&",
            "arch.memory_cpu&.ports.out_noc"
        },
        {
            "arch.noc.ports.out_memory_cpu&",
            "arch.memory_cpu&.ports.in_noc"
        },
        {
            "arch.memory_cpu&.llc^.ports.in_tile_req",
            "arch.memory_cpu&.ports.out_llc^"
        },
        {
            "arch.memory_cpu&.llc^.ports.out_tile_ack",
            "arch.memory_cpu&.ports.in_llc^"
        },
        {
            "arch.memory_cpu&.llc^.ports.out_tile_req",
            "arch.memory_cpu&.ports.in_llc_mc^"
        },
        {
            "arch.memory_cpu&.llc^.ports.in_tile_ack",
            "arch.memory_cpu&.ports.out_llc_mc^"
        },
        {
            "arch.memory_cpu&.ports.out_mc",
            "arch.memory_controller#.ports.in_mcpu"
        },
        {
            "arch.memory_cpu&.ports.in_mc",
            "arch.memory_controller#.ports.out_mcpu"
        }
    };
    
    shared_units = {
            {
                "noc",
                "arch",
                "NoC",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                NULL                                // This is filled in CPUFactory when it is instantiated
            },
    };
}

spike_model::L2TestTopology::L2TestTopology(){

//! Instantiating units of this topology
    units = {
            {
                "l2_bank",
                "arch",
                "L2 Cache Bank",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->cache_bank_rf
            },
    };
    //! Instantiating ports of this topology
    port_connections = {
        {
            "arch.ports.out_tile_req",
            "arch.ports.in_tile_ack",
        },
    };
}

spike_model::MemoryControllerTestTopology::MemoryControllerTestTopology(){

//! Instantiating units of this topology
    units = {
            {
                "memory_controller",
                "arch",
                "Memory Controller",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->memory_controller_rf
            },
            {
                "memory_bank!",
                "arch.memory_controller",
                "Memory Bank !",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->memory_bank_rf
            },

    };
    //! Instantiating ports of this topology
    port_connections = {
    };
}
/*
 * @brief Static method to allocate memory for topology
 */
auto spike_model::CPUTopology::allocateTopology(const std::string& topology) -> spike_model::CPUTopology*{
    CPUTopology* new_topology {nullptr};
    if(topology == "tiled")
    {
        new_topology = new spike_model::TiledTopology();
    }
    else if(topology == "l2_unit_test")
    {
        new_topology = new spike_model::L2TestTopology();
    }
    else if(topology == "memory_controller_unit_test")
    {
        new_topology = new spike_model::MemoryControllerTestTopology();
    }
    else{
        throw sparta::SpartaException("This topology in unrecognized.");
    }
    sparta_assert(new_topology);
    return new_topology;
}
