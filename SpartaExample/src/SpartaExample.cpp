// <SpartaExample.cpp> -*- C++ -*-


#include <iostream>

#include "SpartaExample.hpp"

#include "sparta/simulation/Clock.hpp"
#include "sparta/utils/TimeManager.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/simulation/TreeNodeExtensions.hpp"
#include "sparta/trigger/ContextCounterTrigger.hpp"
#include "sparta/utils/StringUtils.hpp"
#include "sparta/statistics/CycleHistogram.hpp"
#include "sparta/statistics/Histogram.hpp"
#include "sparta/statistics/HistogramFunctionManager.hpp"
#include "sparta/utils/SpartaTester.hpp"

#include "Core.hpp"
#include "Cache.hpp"
#include "Memory.hpp"

namespace minimum_two_phase_example
{
    SpartaExample::SpartaExample(sparta::Scheduler & scheduler) :
        sparta::app::Simulation("sparta_model", &scheduler)
    {
        getResourceSet()->addResourceFactory<sparta::ResourceFactory<Core, Core::CoreParameterSet>>();
        getResourceSet()->addResourceFactory<sparta::ResourceFactory<Cache, Cache::CacheParameterSet>>();
        getResourceSet()->addResourceFactory<sparta::ResourceFactory<Memory, Memory::MemoryParameterSet>>();
    }


    SpartaExample::~SpartaExample()
    {
        getRoot()->enterTeardown();
    }


    template <typename DataT>
    void validateParameter(const sparta::ParameterSet & params,
                           const std::string & param_name,
                           const DataT & expected_value)
    {
        if (!params.hasParameter(param_name)) {
            return;
        }
        const DataT actual_value = params.getParameterValueAs<DataT>(param_name);
        if (actual_value != expected_value) {
            throw sparta::SpartaException("Invalid extension parameter encountered:\n")
                << "\tParameter name:             " << param_name
                << "\nParameter value (actual):   " << actual_value
                << "\nParameter value (expected): " << expected_value;
        }
    }

    template <typename DataT>
    void validateParameter(const sparta::ParameterSet & params,
                           const std::string & param_name,
                           const std::set<DataT> & expected_values)
    {
        bool found = false;
        for (const auto & expected : expected_values) {
            try {
                found = false;
                validateParameter<DataT>(params, param_name, expected);
                found = true;
                break;
            } catch (...) {
            }
        }

        if (!found) {
            throw sparta::SpartaException("Invalid extension parameter "
                                      "encountered for '") << param_name << "'";
        }
    }

    /* This method creates all the Sparta units in the simulator. In the actual Coyote, the flow is a bit harder to follow.
     * It implements a Factory design pattern in combination with the yml files and wild card replacement to achieve flexibility. 
     * The classes involved in this process are CPUFactory and CPUTopology 
     */
    void SpartaExample::buildTree_()
    {
        sparta::ResourceTreeNode* rtn=new sparta::ResourceTreeNode(getRoot(), "core",
                                       sparta::TreeNode::GROUP_NAME_NONE,
                                       sparta::TreeNode::GROUP_IDX_NONE,
                                       "Core",
                                       getResourceSet()->getResourceFactory(Core::name));
        to_delete_.emplace_back(rtn);

        rtn=new sparta::ResourceTreeNode(getRoot(), "cache",
                                       sparta::TreeNode::GROUP_NAME_NONE,
                                       sparta::TreeNode::GROUP_IDX_NONE,
                                       "Cache",
                                       getResourceSet()->getResourceFactory(Cache::name));
        to_delete_.emplace_back(rtn);
        
        rtn=new sparta::ResourceTreeNode(getRoot(), "memory",
                                       sparta::TreeNode::GROUP_NAME_NONE,
                                       sparta::TreeNode::GROUP_IDX_NONE,
                                       "Memory",
                                       getResourceSet()->getResourceFactory(Memory::name));
        to_delete_.emplace_back(rtn);

    }

    void SpartaExample::configureTree_()
    {
    }


    /* This method connects the ports of the Sparta units in the simulator. In the actual Coyote, the flow is a bit harder to follow.
     * It implements a Factory design pattern in combination with the yml files to achieve and wildcard replacement flexibility. The classes involved
     * in this process are CPUFactory and CPUTopology 
     */
    void SpartaExample::bindTree_()
    {
        sparta::TreeNode* root_tree_node = getRoot();
        sparta_assert(root_tree_node != nullptr);

        sparta::bind(root_tree_node->getChildAs<sparta::Port>("core.ports.out_reqs"),
                   root_tree_node->getChildAs<sparta::Port>("cache.ports.in_reqs"));

        sparta::bind(root_tree_node->getChildAs<sparta::Port>("cache.ports.out_reqs"),
                   root_tree_node->getChildAs<sparta::Port>("memory.ports.in_reqs"));
        
        sparta::bind(root_tree_node->getChildAs<sparta::Port>("memory.ports.out_acks"),
                   root_tree_node->getChildAs<sparta::Port>("cache.ports.in_acks"));
        
        sparta::bind(root_tree_node->getChildAs<sparta::Port>("cache.ports.out_acks"),
                   root_tree_node->getChildAs<sparta::Port>("core.ports.in_acks"));
    }
}
