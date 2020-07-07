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
            "core*",
            "cpu",
            "Core *",
            sparta::TreeNode::GROUP_NAME_NONE,
            sparta::TreeNode::GROUP_IDX_NONE,
            &factories->core_rf
        },
    };
    //! Instantiating ports of this topology
    port_connections = {
        {
            "cpu.l2_bank*.ports.out_biu_req",
            "cpu.l2_bank*.ports.in_biu_ack"
        },
        {
            "cpu.noc.ports.out_l2_bank*_req",
            "cpu.l2_bank*.ports.in_noc_req"
        },
        {
            "cpu.noc.ports.in_l2_bank*_ack", 
            "cpu.l2_bank*.ports.out_noc_ack"
        },
        {
            "cpu.noc.ports.in_core*",
            "cpu.core*.ports.out_port"
        },
        {
            "cpu.noc.ports.out_core*", 
            "cpu.core*.ports.in_port"
        },
    };

    shared_units = {
            {
                "l2_bank*",
                "cpu",
                "L2 Cache Bank *",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->l2_rf
            },
            {
                "noc",
                "cpu",
                "NoC",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->noc_rf
            },
            {
                "spike",
                "cpu",
                "Spike",
                sparta::TreeNode::GROUP_NAME_NONE,
                sparta::TreeNode::GROUP_IDX_NONE,
                &factories->spike_rf
            }
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
