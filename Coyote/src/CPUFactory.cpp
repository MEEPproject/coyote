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

// <CPUFactory.cpp> -*- C++ -*-


#include "CPUFactory.hpp"
#include <string>
#include <algorithm>
#include <sparta/app/Simulation.hpp>
#include <sparta/app/SimulationConfiguration.hpp>

/**
 * @brief Constructor for CPUFactory
 */
coyote::CPUFactory::CPUFactory() :
    sparta::ResourceFactory<coyote::CPU, coyote::CPU::CPUParameterSet>(){}

/**
 * @brief Destructor for CPUFactory
 */
coyote::CPUFactory::~CPUFactory() = default;

/**
 * @brief Set the user-defined topology for this microarchitecture
 */
auto coyote::CPUFactory::setTopology(const std::string& topology,
                                          const uint32_t num_tiles,
                                          const uint32_t num_banks_per_tile,
                                          const uint32_t num_memory_cpus,
                                          const uint32_t num_llcs,
                                          const uint32_t num_memory_controllers,
                                          const uint32_t num_memory_banks,
                                          const bool trace) -> void{
    sparta_assert(!topology_);
    topology_.reset(coyote::CPUTopology::allocateTopology(topology));
    topology_->setName(topology);
    topology_->setNumTiles(num_tiles);
    topology_->setNumL2BanksPerTile(num_banks_per_tile);
    topology_->setNumMemoryCPUs(num_memory_cpus);
    topology_->setNumLLCs(num_llcs);
    topology_->setNumMemoryControllers(num_memory_controllers);
    topology_->setNumMemoryBanksPerMemoryController(num_memory_banks);
    topology_->setTrace(trace);
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}



/**
 * @brief Implemenation : Build the device tree by instantiating resource nodes
 */
auto coyote::CPUFactory::buildTree_(sparta::RootTreeNode* root_node,
                                          const std::vector<coyote::CPUTopology::UnitInfo>& units) -> void{
    std::string parent_name, human_name, node_name, replace_with;
    for(const auto& unit : units){
        parent_name = unit.parent_name;
        node_name = unit.name;
        human_name = unit.human_name;
        std::cout << "Handling " << node_name << "\n";
        if(node_name.find(to_replace_tiles_)!=std::string::npos)
        {
            for(std::size_t num_of_tiles = 0; num_of_tiles < topology_->num_tiles; ++num_of_tiles){
                parent_name = unit.parent_name;
                node_name = unit.name;
                human_name = unit.human_name;
                replace_with = std::to_string(num_of_tiles);
                replace(parent_name, to_replace_tiles_, replace_with);
                replace(node_name, to_replace_tiles_, replace_with);
                replace(human_name, to_replace_tiles_, replace_with);
                auto parent_node = root_node->getChildAs<sparta::TreeNode>(parent_name);
                auto rtn = new sparta::ResourceTreeNode(parent_node,
                                                      node_name,
                                                      unit.group_name,
                                                      unit.group_id,
                                                      human_name,
                                                      unit.factory);
                if(unit.is_private_subtree){
                    rtn->makeSubtreePrivate();
                    private_nodes_.emplace_back(rtn);
                }
                to_delete_.emplace_back(rtn);
                resource_names_.emplace_back(node_name);
            }
        }
        else if(node_name.find(to_replace_banks_)!=std::string::npos)
        {
            for(std::size_t num_of_tiles = 0; num_of_tiles < topology_->num_tiles; ++num_of_tiles){
                for(std::size_t num_of_banks = 0; num_of_banks < topology_->num_banks_per_tile; ++num_of_banks){
                    parent_name = unit.parent_name;
                    node_name = unit.name;
                    human_name = unit.human_name;
                    replace(parent_name, to_replace_tiles_, std::to_string(num_of_tiles));
                    replace(node_name, to_replace_banks_, std::to_string(num_of_banks));
                    replace(human_name, to_replace_banks_, std::to_string(num_of_banks));
                    auto parent_node = root_node->getChildAs<sparta::TreeNode>(parent_name);
                    auto rtn = new sparta::ResourceTreeNode(parent_node,
                                                          node_name,
                                                          unit.group_name,
                                                          unit.group_id,
                                                          human_name,
                                                          unit.factory);
                    if(unit.is_private_subtree){
                        rtn->makeSubtreePrivate();
                        private_nodes_.emplace_back(rtn);
                    }
                    to_delete_.emplace_back(rtn);
                    resource_names_.emplace_back(node_name);
                }
            }
        }
        else if(node_name.find(to_replace_memory_cpus_)!=std::string::npos)
        {
            for(std::size_t num_of_memory_cpus = 0; num_of_memory_cpus < topology_->num_memory_cpus; ++num_of_memory_cpus) {
                parent_name = unit.parent_name;
                node_name = unit.name;
                human_name = unit.human_name;
                replace_with = std::to_string(num_of_memory_cpus);
                replace(parent_name, to_replace_memory_cpus_, replace_with);
                replace(node_name, to_replace_memory_cpus_, replace_with);
                replace(human_name, to_replace_memory_cpus_, replace_with);
                auto parent_node = root_node->getChildAs<sparta::TreeNode>(parent_name);
                auto rtn = new sparta::ResourceTreeNode(parent_node,
                                                      node_name,
                                                      unit.group_name,
                                                      unit.group_id,
                                                      human_name,
                                                      unit.factory);
                if(unit.is_private_subtree){
                    rtn->makeSubtreePrivate();
                    private_nodes_.emplace_back(rtn);
                }
                to_delete_.emplace_back(rtn);
                resource_names_.emplace_back(node_name);
            }
        }
        else if(node_name.find(to_replace_llc_)!=std::string::npos)
        {
            for(std::size_t num_of_memory_cpus = 0; num_of_memory_cpus < topology_->num_memory_cpus; ++num_of_memory_cpus) 
            {
                for(std::size_t num_of_llcs = 0; num_of_llcs < topology_->num_llcs; ++num_of_llcs) 
                {
                    parent_name = unit.parent_name;
                    node_name = unit.name;
                    human_name = unit.human_name;
                    replace_with = std::to_string(num_of_llcs);
                    replace(parent_name, to_replace_memory_cpus_, std::to_string(num_of_memory_cpus));
                    replace(node_name, to_replace_llc_, replace_with);
                    replace(human_name, to_replace_llc_, replace_with);
                    auto parent_node = root_node->getChildAs<sparta::TreeNode>(parent_name);
                    auto rtn = new sparta::ResourceTreeNode(parent_node,
                                          node_name,
                                          unit.group_name,
                                          unit.group_id,
                                          human_name,
                                          unit.factory);
                    if(unit.is_private_subtree)
                    {
                        rtn->makeSubtreePrivate();
                        private_nodes_.emplace_back(rtn);
                    }
                    to_delete_.emplace_back(rtn);
                    resource_names_.emplace_back(node_name);
                }
	        }
        }
        else if(node_name.find(to_replace_memory_controllers_)!=std::string::npos)
        {
            for(std::size_t num_of_memory_controllers = 0; num_of_memory_controllers < topology_->num_memory_controllers; ++num_of_memory_controllers){
                parent_name = unit.parent_name;
                node_name = unit.name;
                human_name = unit.human_name;
                replace_with = std::to_string(num_of_memory_controllers);
                replace(parent_name, to_replace_memory_controllers_, replace_with);
                replace(node_name, to_replace_memory_controllers_, replace_with);
                replace(human_name, to_replace_memory_controllers_, replace_with);
                auto parent_node = root_node->getChildAs<sparta::TreeNode>(parent_name);
                auto rtn = new sparta::ResourceTreeNode(parent_node,
                                                      node_name,
                                                      unit.group_name,
                                                      unit.group_id,
                                                      human_name,
                                                      unit.factory);
                if(unit.is_private_subtree){
                    rtn->makeSubtreePrivate();
                    private_nodes_.emplace_back(rtn);
                }
                to_delete_.emplace_back(rtn);
                resource_names_.emplace_back(node_name);
            }
        }
        else if(node_name.find(to_replace_memory_banks_)!=std::string::npos)
        {
            for(std::size_t num_of_memory_controllers = 0; num_of_memory_controllers < topology_->num_memory_controllers; ++num_of_memory_controllers){
                for(std::size_t num_of_memory_banks = 0; num_of_memory_banks < topology_->num_memory_banks; ++num_of_memory_banks){
                    parent_name = unit.parent_name;
                    node_name = unit.name;
                    human_name = unit.human_name;
                    replace_with = std::to_string(num_of_memory_banks);
                    replace(parent_name, to_replace_memory_controllers_, std::to_string(num_of_memory_controllers));
                    replace(node_name, to_replace_memory_banks_, replace_with);
                    replace(human_name, to_replace_memory_banks_, replace_with);
                    auto parent_node = root_node->getChildAs<sparta::TreeNode>(parent_name);
                    auto rtn = new sparta::ResourceTreeNode(parent_node,
                                                          node_name,
                                                          unit.group_name,
                                                          unit.group_id,
                                                          human_name,
                                                          unit.factory);
                    if(unit.is_private_subtree){
                        rtn->makeSubtreePrivate();
                        private_nodes_.emplace_back(rtn);
                    }
                    to_delete_.emplace_back(rtn);
                    resource_names_.emplace_back(node_name);
                }
            }
        }
        else if(node_name.find("arbiter")!=std::string::npos)
        {
            for(std::size_t num_of_tiles = 0; num_of_tiles < topology_->num_tiles; ++num_of_tiles){
                parent_name = unit.parent_name;
                node_name = unit.name;
                human_name = unit.human_name;
                replace_with = std::to_string(num_of_tiles);
                replace(parent_name, to_replace_tiles_, replace_with);
                replace(node_name, to_replace_arbiter_, "");
                replace(human_name, to_replace_arbiter_, replace_with);
                auto parent_node = root_node->getChildAs<sparta::TreeNode>(parent_name);
                auto rtn = new sparta::ResourceTreeNode(parent_node,
                                                      node_name,
                                                      unit.group_name,
                                                      unit.group_id,
                                                      human_name,
                                                      unit.factory);
                if(unit.is_private_subtree){
                    rtn->makeSubtreePrivate();
                    private_nodes_.emplace_back(rtn);
                }
                to_delete_.emplace_back(rtn);
                resource_names_.emplace_back(node_name);
            }
        }
        else
        {
            std::cout << "Adding resource "<< node_name <<" with no name substitution\n";
            parent_name = unit.parent_name;
            node_name = unit.name;
            human_name = unit.human_name;
            auto parent_node = root_node->getChildAs<sparta::TreeNode>(parent_name);
            auto rtn = new sparta::ResourceTreeNode(parent_node,
                                                  node_name,
                                                  unit.group_name,
                                                  unit.group_id,
                                                  human_name,
                                                  unit.factory);
            to_delete_.emplace_back(rtn);
            resource_names_.emplace_back(node_name);
        }
    }
}

auto coyote::CPUFactory::buildTreeShared_(sparta::RootTreeNode* root_node,
                                          const std::vector<coyote::CPUTopology::UnitInfo>& units) -> void{
    std::string parent_name, human_name, node_name, replace_with, og_name;
    for(const auto& unit : units)
    {
        for(std::size_t num_of_l2_banks = 0; num_of_l2_banks < topology_->num_banks_per_tile; ++num_of_l2_banks)
        {
            //FIX: This code assumes that this buildTreeShared is only for NoC
            const std::string noc_model = root_node->getRoot()->getAs<sparta::RootTreeNode>()->getSimulator()->getSimulationConfiguration()->getUnboundParameterTree().tryGet("top.arch.noc.params.noc_model")->getAs<std::string>();
            sparta::ResourceFactoryBase* noc_rf=unit.factory;
            if(noc_model == "functional")
                noc_rf = &topology_->factories->functional_noc_rf;
            else if(noc_model == "simple")
                noc_rf = &topology_->factories->simple_noc_rf;
            else if(noc_model == "detailed")
                noc_rf = &topology_->factories->detailed_noc_rf;
            else
                sparta_assert(false, "top.arch.noc.params.noc_model must be: functional, simple or detailed. Not: " << noc_model);
            og_name=unit.name;
            parent_name = unit.parent_name;
            node_name = unit.name;
            human_name = unit.human_name;
            replace_with = std::to_string(num_of_l2_banks);
            replace(parent_name, to_replace_tiles_, replace_with);
            replace(node_name, to_replace_tiles_, replace_with);
            replace(human_name, to_replace_tiles_, replace_with);
            auto parent_node = root_node->getChildAs<sparta::TreeNode>(parent_name);
            sparta_assert(noc_rf!=nullptr);
            auto rtn = new sparta::ResourceTreeNode(parent_node,
                    node_name,
                    unit.group_name,
                    unit.group_id,
                    human_name,
                    noc_rf);
            if(unit.is_private_subtree){
                rtn->makeSubtreePrivate();
                private_nodes_.emplace_back(rtn);
            }
            to_delete_.emplace_back(rtn);
            resource_names_.emplace_back(node_name);

            if(node_name==og_name) //There is an only resource of this kind, so go to next
            {
                std::cout << "Breaking on " << node_name << "\n";
                break;
            }
        }
    }
}

/**
 * @brief Implementation : Bind all the ports between different units and set TLBs and preload
 */
auto coyote::CPUFactory::bindTree_(sparta::RootTreeNode* root_node,
                                         const std::vector<coyote::CPUTopology::PortConnectionInfo>& ports) -> void{
    std::string out_port_name, in_port_name,replace_with;

    if(topology_->topology_name=="tiled")
    {
        NoC * noc=NULL;

        if(topology_->num_tiles!=0)
        {
            noc=root_node->getChild(std::string("arch.noc"))->getResourceAs<coyote::NoC>();

            //CacheBank * bank=root_node->getChild(std::string("arch.tile0.l2_bank0"))->getResourceAs<coyote::CacheBank>();


            if(topology_->trace)
            {
                noc->setLogger(&topology_->logger);
            }
        }

        for(std::size_t num_of_tiles = 0; num_of_tiles < topology_->num_tiles; ++num_of_tiles)
        {
            bool bind = false;
            replace_with = std::to_string(num_of_tiles);
            for(const auto& port : ports)
            {
                out_port_name = port.output_port_name;
                in_port_name = port.input_port_name;

                if (in_port_name.find("arbiter") != std::string::npos)
                {
                    replace(in_port_name, to_replace_arbiter_, "");
                    replace(in_port_name, to_replace_tiles_, replace_with);
                    bind=true;
                }
                if (out_port_name.find("arbiter") != std::string::npos)
                {
                    replace(out_port_name, to_replace_tiles_, replace_with);
                    replace(out_port_name, to_replace_arbiter_, "");
                    bind=true;
                }
                if(bind && !root_node->getChildAs<sparta::Port>(in_port_name)->isBound())
                {
                    std::cout << "Binding " << out_port_name << " and " << in_port_name << "\n";
                    sparta::bind(root_node->getChildAs<sparta::Port>(in_port_name),
                    root_node->getChildAs<sparta::Port>(out_port_name));
                    break;
                }
            }
        }

        for(std::size_t num_of_tiles = 0; num_of_tiles < topology_->num_tiles; ++num_of_tiles)
        {
            for(std::size_t num_of_banks = 0; num_of_banks < topology_->num_banks_per_tile; ++num_of_banks)
            {
                for(const auto& port : ports)
                {
                    bool bind=false;
                    out_port_name = port.output_port_name;
                    in_port_name = port.input_port_name;
                    replace_with = std::to_string(num_of_banks);

                    if (out_port_name.find("l2_bank$") != std::string::npos)
                    {
                        replace(out_port_name, to_replace_banks_, replace_with);
                        bind=true;
                    }

                    if (in_port_name.find("l2_bank$") != std::string::npos)
                    {
                        replace(in_port_name, to_replace_banks_, replace_with);
                        bind=true;
                    }

                    replace_with = std::to_string(num_of_tiles);
                    if (out_port_name.find("tile*") != std::string::npos)
                    {
                        replace(out_port_name, to_replace_tiles_, replace_with);
                        bind=true;
                    }

                    if (in_port_name.find("tile*") != std::string::npos)
                    {
                        replace(in_port_name, to_replace_tiles_, replace_with);
                        bind=true;
                    }
                    
                    if(bind)
                    {
                        if(!root_node->getChildAs<sparta::Port>(out_port_name)->isBound())
                        {
                            std::cout << "Binding " << out_port_name << " and " << in_port_name << "\n";
                            sparta::bind(root_node->getChildAs<sparta::Port>(out_port_name),
                                root_node->getChildAs<sparta::Port>(in_port_name));
                        }
                    }
                }
            }
        }

        
        for(std::size_t num_of_memory_controllers = 0; num_of_memory_controllers < topology_->num_memory_controllers; ++num_of_memory_controllers)
        {
                auto mc_node = root_node->getChild(std::string("arch.memory_controller") +
                        sparta::utils::uint32_to_str(num_of_memory_controllers));
                sparta_assert(mc_node != nullptr);
                MemoryController *mc = mc_node->getResourceAs<coyote::MemoryController>();

                for(std::size_t num_of_memory_banks = 0; num_of_memory_banks < topology_->num_memory_banks; ++num_of_memory_banks) {
                    auto bank_node = root_node->getChild(std::string("arch.memory_controller") +
                            sparta::utils::uint32_to_str(num_of_memory_controllers) +
                std::string(".memory_bank") + sparta::utils::uint32_to_str(num_of_memory_banks));
                    sparta_assert(bank_node != nullptr);
                    MemoryBank * b=bank_node->getResourceAs<coyote::MemoryBank>();
                    mc->addBank_(b);
                    b->setMemoryController(mc);
                }
        }
        
        coyote::NoC *noc_for_mcpu = nullptr;
        if(topology_->num_memory_cpus!=0)
        {
            noc_for_mcpu = root_node->getChild(std::string("arch.noc"))->getResourceAs<coyote::NoC>();
                        
            //-- create an array of memory tiles and give it to the NoC
            std::shared_ptr<std::vector<MemoryCPUWrapper *>> memory_tiles = std::make_shared<std::vector<MemoryCPUWrapper *>>();
            for(std::size_t num_of_memory_cpus = 0; num_of_memory_cpus < topology_->num_memory_cpus; ++num_of_memory_cpus) {
                auto mcpu_node = root_node->getChild(std::string("arch.memory_cpu") +
                        sparta::utils::uint32_to_str(num_of_memory_cpus));
                sparta_assert(mcpu_node != nullptr);

                MemoryCPUWrapper *mcpu = mcpu_node->getResourceAs<coyote::MemoryCPUWrapper>();
                memory_tiles->push_back(mcpu);
            }
            
            noc_for_mcpu->setMemoryTiles(memory_tiles);
        }

        for(std::size_t num_of_memory_cpus = 0; num_of_memory_cpus < topology_->num_memory_cpus; ++num_of_memory_cpus) 
        {
            auto mcpu_node = root_node->getChild(std::string("arch.memory_cpu") + 
                sparta::utils::uint32_to_str(num_of_memory_cpus));
            sparta_assert(mcpu_node != nullptr);

            MemoryCPUWrapper *mcpu = mcpu_node->getResourceAs<coyote::MemoryCPUWrapper>();
            mcpu->setID(num_of_memory_cpus);
            mcpu->setNoC(noc_for_mcpu);
            
            /*auto mc_node = root_node->getChild(std::string("arch.memory_controller") +
                    sparta::utils::uint32_to_str(num_of_memory_cpus));                  // the MPCU is bound to one MC
            sparta_assert(mc_node != nullptr);
            */

            //-- Bind the ports
            for(std::size_t num_of_llcs = 0; num_of_llcs < topology_->num_llcs; ++num_of_llcs)
            {
                for(const auto& port : ports) 
                {
                    bool bind = false;
                    out_port_name = port.output_port_name;
                    in_port_name  = port.input_port_name;
                    replace_with  = std::to_string(num_of_memory_cpus);
                    if(out_port_name.find("memory_cpu&") != std::string::npos) {
                        replace(out_port_name, to_replace_memory_cpus_, replace_with);
                        bind = true;
                    }
                    if(in_port_name.find("memory_cpu&") != std::string::npos) {
                        replace(in_port_name, to_replace_memory_cpus_, replace_with);
                        bind = true;
                    }
                    if(out_port_name.find("llc^") != std::string::npos) {
                        replace(out_port_name, to_replace_llc_, std::to_string(num_of_llcs));
                        bind = true;
                    }
                    if (in_port_name.find("llc_mc^") != std::string::npos){
                                    replace(in_port_name, to_replace_llc_, std::to_string(num_of_llcs));
                                    bind=true;
                            }
                    if(in_port_name.find("llc^") != std::string::npos) {
                        replace(in_port_name, to_replace_llc_, std::to_string(num_of_llcs));
                        bind = true;
                    }
                            if(out_port_name.find("memory_controller#") != std::string::npos) {
                            replace(out_port_name, to_replace_memory_controllers_, replace_with);
                                bind = true;
                            }
                        if(in_port_name.find("memory_controller#") != std::string::npos) {
                                replace(in_port_name, to_replace_memory_controllers_, replace_with);
                                bind = true;
                            }

                    if(bind && !root_node->getChildAs<sparta::Port>(out_port_name)->isBound()) {
                        std::cout << "Binding " << out_port_name << " and " << in_port_name << std::endl;
                        sparta::bind(root_node->getChildAs<sparta::Port>(out_port_name), root_node->getChildAs<sparta::Port>(in_port_name));
                    }
                }
            }
        }
        std::cout << "Binding done " << std::endl;

        for(std::size_t num_of_tiles = 0; num_of_tiles < topology_->num_tiles; ++num_of_tiles){
            auto core_node = root_node->getChild(std::string("arch.tile") +
                    sparta::utils::uint32_to_str(num_of_tiles));
            sparta_assert(core_node != nullptr);

            Tile * tile=core_node->getResourceAs<coyote::Tile>();

            tile->setId(num_of_tiles);

            auto arbiter_node = root_node->getChild(std::string("arch.tile") + sparta::utils::uint32_to_str(num_of_tiles)
                                          + std::string(".arbiter"));
            sparta_assert(arbiter_node != nullptr);
            Arbiter *arbiter = arbiter_node->getResourceAs<coyote::Arbiter>();
            tile->setArbiter(arbiter);
            arbiter->setNoC(noc);

            for(int i = 0; i < tile->getL2Banks();i++)
            {
                auto l2_node = root_node->getChild(std::string("arch.tile") + sparta::utils::uint32_to_str(num_of_tiles) +
                            std::string(".l2_bank") + sparta::utils::uint32_to_str(i));
                sparta_assert(l2_node != nullptr);
                L2CacheBank* bank  = l2_node->getResourceAs<coyote::L2CacheBank>();
                bank->setTile(tile);
                arbiter->addBank(bank);
            }

            /*if(topology_->trace)
            {
                core->setLogger(&topology_->logger);
            }*/
        }

        if(topology_->trace)
        {
            for(std::size_t num_of_tiles = 0; num_of_tiles < topology_->num_tiles; ++num_of_tiles)
            {
                auto tile_node = root_node->getChild(std::string("arch.tile") +
                        sparta::utils::uint32_to_str(num_of_tiles));
                sparta_assert(tile_node != nullptr);

                Tile * tile=tile_node->getResourceAs<coyote::Tile>();

                tile->setLogger(&topology_->logger);

                for(std::size_t num_of_l2_banks = 0; num_of_l2_banks < topology_->num_banks_per_tile; ++num_of_l2_banks){
                    auto bank_node = root_node->getChild(std::string("arch.tile") + sparta::utils::uint32_to_str(num_of_tiles) +
                            std::string(".l2_bank") + sparta::utils::uint32_to_str(num_of_l2_banks));
                    sparta_assert(bank_node != nullptr);

                    L2CacheBank * bank=bank_node->getResourceAs<coyote::L2CacheBank>();

                    bank->setLogger(&topology_->logger);
                }
                auto arbiter_node = root_node->getChild(std::string("arch.tile") + sparta::utils::uint32_to_str(num_of_tiles) +
                        std::string(".arbiter"));
                sparta_assert(arbiter_node != nullptr);

                Arbiter * arb=arbiter_node->getResourceAs<coyote::Arbiter>();

                arb->setLogger(&topology_->logger);
            }

            for(std::size_t num_of_memory_cpus = 0; num_of_memory_cpus < topology_->num_memory_cpus; ++num_of_memory_cpus) {
                auto mcpu_node = root_node->getChild(std::string("arch.memory_cpu") +
                        sparta::utils::uint32_to_str(num_of_memory_cpus));
                sparta_assert(mcpu_node != nullptr);

                MemoryCPUWrapper *mcpu = mcpu_node->getResourceAs<coyote::MemoryCPUWrapper>();
                mcpu->setLogger(&topology_->logger);
            }

            for(std::size_t num_of_memory_controllers = 0; num_of_memory_controllers < topology_->num_memory_controllers; ++num_of_memory_controllers)
            {
                auto mc_node = root_node->getChild(std::string("arch.memory_controller") +
                        sparta::utils::uint32_to_str(num_of_memory_controllers));
                sparta_assert(mc_node != nullptr);

                MemoryController * mc=mc_node->getResourceAs<coyote::MemoryController>();

                mc->setLogger(&topology_->logger);

                for(std::size_t num_of_memory_banks = 0; num_of_memory_banks < topology_->num_memory_banks; ++num_of_memory_banks) {
                    auto bank_node = root_node->getChild(std::string("arch.memory_controller") +
                            sparta::utils::uint32_to_str(num_of_memory_controllers) +
                std::string(".memory_bank") + sparta::utils::uint32_to_str(num_of_memory_banks));
                    sparta_assert(bank_node != nullptr);
                    MemoryBank * b=bank_node->getResourceAs<coyote::MemoryBank>();
                    b->setLogger(&topology_->logger);
                }
            }
        }
    }
    else if(topology_->topology_name=="l2_unit_test")
    {
        //I should be binding here
        auto bank_node = root_node->getChild(std::string("arch.l2_bank"));
        sparta_assert(bank_node != nullptr);

        L2CacheBank * bank=bank_node->getResourceAs<coyote::L2CacheBank>();

        bank->setLogger(&topology_->logger);
                        
        sparta::bind(root_node->getChildAs<sparta::Port>("arch.l2_bank.ports.out_tile_req"), root_node->getChildAs<sparta::Port>("arch.l2_bank.ports.in_tile_ack"));
    }
    else if(topology_->topology_name=="memory_controller_unit_test")
    {
        //I should be binding here
        auto mc_node = root_node->getChild(std::string("arch.memory_controller"));
        sparta_assert(mc_node != nullptr);

        MemoryController * mc=mc_node->getResourceAs<coyote::MemoryController>();

        mc->setLogger(&topology_->logger);
        for(std::size_t num_of_memory_banks = 0; num_of_memory_banks < topology_->num_memory_banks; ++num_of_memory_banks) {
            auto bank_node = root_node->getChild(std::string("arch.memory_controller") + std::string(".memory_bank") + sparta::utils::uint32_to_str(num_of_memory_banks));
            sparta_assert(bank_node != nullptr);
            MemoryBank * b=bank_node->getResourceAs<coyote::MemoryBank>();
            b->setLogger(&topology_->logger);
            mc->addBank_(b);
            b->setMemoryController(mc);
        }
        

        auto bank_node = root_node->getChild(std::string("arch.memory_controller.memory_bank0"));
        MemoryBank * b=bank_node->getResourceAs<coyote::MemoryBank>();
        uint64_t num_rows=b->getNumRows();
        uint64_t num_cols=b->getNumColumns();

        mc->setup_masks_and_shifts_(1, num_rows, num_cols);
    }
    else if(topology_->topology_name=="l2_unit_test")
    {
        //I should be binding here
        auto bank_node = root_node->getChild(std::string("arch.l2_bank"));
        sparta_assert(bank_node != nullptr);

        L2CacheBank * bank=bank_node->getResourceAs<coyote::L2CacheBank>();

        bank->setLogger(&topology_->logger);
                        
        sparta::bind(root_node->getChildAs<sparta::Port>("arch.l2_bank.ports.out_tile_req"), root_node->getChildAs<sparta::Port>("arch.l2_bank.ports.in_tile_ack"));
    }
}

/**
 * @brief Build the device tree by instantiating resource nodes
 */
auto coyote::CPUFactory::buildTree(sparta::RootTreeNode* root_node) -> void{
    sparta_assert(topology_);
    buildTree_(root_node, topology_->units);
    buildTreeShared_(root_node, topology_->shared_units);
}

/**
 * @brief Bind all the ports between different units and set TLBs and preload
 */
auto coyote::CPUFactory::bindTree(sparta::RootTreeNode* root_node) -> void{
    sparta_assert(topology_);
    bindTree_(root_node, topology_->port_connections);
}

/**
 * @brief Get the list of resources instantiated in this topology
 */
auto coyote::CPUFactory::getResourceNames() const -> const std::vector<std::string>&{
    return resource_names_;
}

coyote::Logger * coyote::CPUFactory::getLogger()
{
    return &topology_->logger;
}
// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab:
