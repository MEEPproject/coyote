// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 

// <CPUFactory.h> -*- C++ -*-


#ifndef __CPU_FACTORY_H__
#define __CPU_FACTORY_H__

#include "sparta/simulation/ResourceFactory.hpp"
#include "sparta/simulation/RootTreeNode.hpp"
//#include "CPU.hpp"
#include "sparta/simulation/ResourceTreeNode.hpp"
#include "CPUTopology.hpp"
#include "Logger.hpp"

namespace coyote{

class CPUFactory : public sparta::ResourceFactory<CPU, CPU::CPUParameterSet>{
public:
    /**
     * \class coyote::CPUFactory
     * \brief CPUFactory will act as the place where a user-defined topology
     *        will be translated into actual Resource Treenodes. It is responsible
     *        for binding ports between units.
     *
     * CPUFactory unit will
     * 1. Set recipe for topology and number of cores in processor
     * 2. Build the actual tree nodes
     * 3. Bind the ports of different logical units together
     */

    /**
     * @brief Constructor for CPUFactory
     */
    CPUFactory();

    /**
     * @brief Destructor for CPUFactory
     */
    ~CPUFactory();

    /**
     * @brief Set the user-defined topology for this microarchitecture
     */
    auto setTopology(const std::string&, const uint32_t, const uint32_t, const uint32_t, const uint32_t, const uint32_t, const uint32_t, const bool) -> void;

    /**
     * @brief Build the device tree by instantiating resource nodes
     */
    auto buildTree(sparta::RootTreeNode*) -> void;

    /**
     * @brief Bind all the ports between different units and set TLBs and preload
     */
    auto bindTree(sparta::RootTreeNode*) -> void;

    /**
     * @brief Get the list of resources instantiated in this topology
     */
    auto getResourceNames() const -> const std::vector<std::string>&;

    Logger * getLogger();

private:

    /**
     * @brief Implementation : Build the device tree by instantiating resource nodes
     */
    auto buildTree_(sparta::RootTreeNode*,
                    const std::vector<CPUTopology::UnitInfo>&) -> void;

    /**
     * @brief Implementation : Build the device tree for shared units by instantiating resource nodes
     */
    auto buildTreeShared_(sparta::RootTreeNode* root_node,
                          const std::vector<coyote::CPUTopology::UnitInfo>& units) -> void;
    /**
     * @brief Implementation : Bind all the ports between different units and set TLBs and preload
     */
    auto bindTree_(sparta::RootTreeNode*,
                   const std::vector<CPUTopology::PortConnectionInfo>&) -> void;

    /**
     * @brief Wildcard to be replaced by the multicore idx
     */
    std::string to_replace_tiles_ {"*"};
    std::string to_replace_banks_ {"$"};
    std::string to_replace_memory_cpus_ {"&"};
    std::string to_replace_llc_ {"^"};
    std::string to_replace_memory_controllers_ {"#"};
    std::string to_replace_memory_banks_ {"!"};
    std::string to_replace_arbiter_ {""};

    /**
     * @brief The user-defined topology unit
     */
    std::unique_ptr<CPUTopology> topology_;

    /**
     * @brief Vector of instantiated resource names
     */
    std::vector<std::string> resource_names_;

    /**
     * @brief Vector of private tree nodes
     */
    std::vector<sparta::TreeNode*> private_nodes_;

    /**
     * @brief RAII way of deleting nodes that this class created
     */
    std::vector<std::unique_ptr<sparta::ResourceTreeNode>> to_delete_;
}; // class CPUFactory
}  // namespace coyote
#endif
