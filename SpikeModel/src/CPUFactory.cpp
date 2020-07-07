// <CPUFactory.cpp> -*- C++ -*-


#include "CPUFactory.hpp"
#include <string>
#include <algorithm>

/**
 * @brief Constructor for CPUFactory
 */
spike_model::CPUFactory::CPUFactory() :
    sparta::ResourceFactory<spike_model::CPU, spike_model::CPU::CPUParameterSet>(){}

/**
 * @brief Destructor for CPUFactory
 */
spike_model::CPUFactory::~CPUFactory() = default;

/**
 * @brief Set the user-defined topology for this microarchitecture
 */
auto spike_model::CPUFactory::setTopology(const std::string& topology,
                                           const uint32_t num_cores,
                                            const uint32_t num_l2_banks) -> void{
    sparta_assert(!topology_);
    topology_.reset(spike_model::CPUTopology::allocateTopology(topology));
    topology_->setName(topology);
    topology_->setNumCores(num_cores);
    topology_->setNumL2Banks(num_l2_banks);
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
auto spike_model::CPUFactory::buildTree_(sparta::RootTreeNode* root_node,
                                          const std::vector<spike_model::CPUTopology::UnitInfo>& units) -> void{
    std::string parent_name, human_name, node_name, replace_with;
    for(std::size_t num_of_cores = 0; num_of_cores < topology_->num_cores; ++num_of_cores){
        for(const auto& unit : units){
            parent_name = unit.parent_name;
            node_name = unit.name;
            human_name = unit.human_name;
            replace_with = std::to_string(num_of_cores);
            replace(parent_name, to_replace_, replace_with);
            replace(node_name, to_replace_, replace_with);
            replace(human_name, to_replace_, replace_with);
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

auto spike_model::CPUFactory::buildTreeShared_(sparta::RootTreeNode* root_node,
                                          const std::vector<spike_model::CPUTopology::UnitInfo>& units) -> void{
    std::string parent_name, human_name, node_name, replace_with, og_name;
    for(const auto& unit : units)
    {
        for(std::size_t num_of_l2_banks = 0; num_of_l2_banks < topology_->num_l2_banks; ++num_of_l2_banks)
        {
            og_name=unit.name;
            parent_name = unit.parent_name;
            node_name = unit.name;
            human_name = unit.human_name;
            replace_with = std::to_string(num_of_l2_banks);
            replace(parent_name, to_replace_, replace_with);
            replace(node_name, to_replace_, replace_with);
            replace(human_name, to_replace_, replace_with);
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
auto spike_model::CPUFactory::bindTree_(sparta::RootTreeNode* root_node,
                                         const std::vector<spike_model::CPUTopology::PortConnectionInfo>& ports) -> void{
    std::string out_port_name, in_port_name,replace_with;
    NoC * noc=root_node->getChild(std::string("cpu.noc"))->getResourceAs<spike_model::NoC>();
    SpikeWrapper * spike=root_node->getChild(std::string("cpu.spike"))->getResourceAs<spike_model::SpikeWrapper>();

    L2Cache * bank=root_node->getChild(std::string("cpu.l2_bank0"))->getResourceAs<spike_model::L2Cache>();

    noc->setL2BankInfo(bank->getSize(), bank->getAssoc(), bank->getLineSize());

    for(const auto& port : ports)
    {
        for(std::size_t num_of_l2_banks = 0; num_of_l2_banks < topology_->num_l2_banks; ++num_of_l2_banks)
        {
            bool bind=false;
            out_port_name = port.output_port_name;
            in_port_name = port.input_port_name;
            replace_with = std::to_string(num_of_l2_banks);
            if (out_port_name.find("l2_bank*") != std::string::npos) 
            {
                replace(out_port_name, to_replace_, replace_with);
                bind=true;
            }
            
            if (in_port_name.find("l2_bank*") != std::string::npos)
            {
                replace(in_port_name, to_replace_, replace_with);
                bind=true;
            }
            
            if(bind) 
            {
                sparta::bind(root_node->getChildAs<sparta::Port>(out_port_name),
                    root_node->getChildAs<sparta::Port>(in_port_name));
            }
        }
        
        for(std::size_t num_of_cores = 0; num_of_cores < topology_->num_cores; ++num_of_cores)
        {
            bool bind=false;
            out_port_name = port.output_port_name;
            in_port_name = port.input_port_name;
            replace_with = std::to_string(num_of_cores);
            if (out_port_name.find("core*") != std::string::npos) 
            {
                replace(out_port_name, to_replace_, replace_with);
                bind=true;
            }
            
            if (in_port_name.find("core*") != std::string::npos)
            {
                replace(in_port_name, to_replace_, replace_with);
                bind=true;
            } 
            if(bind)
            {
                sparta::bind(root_node->getChildAs<sparta::Port>(out_port_name),
                        root_node->getChildAs<sparta::Port>(in_port_name));
            }
        } 
    }
       
    for(std::size_t num_of_cores = 0; num_of_cores < topology_->num_cores; ++num_of_cores){
            auto core_node = root_node->getChild(std::string("cpu.core") +
                sparta::utils::uint32_to_str(num_of_cores));
            sparta_assert(core_node != nullptr);

            Core * orch=core_node->getResourceAs<spike_model::Core>();

            orch->setId(num_of_cores);
            orch->setSpike(*spike);

            noc->setOrchestrator(num_of_cores, *orch);
    }
}

/**
 * @brief Build the device tree by instantiating resource nodes
 */
auto spike_model::CPUFactory::buildTree(sparta::RootTreeNode* root_node) -> void{
    sparta_assert(topology_);
    buildTree_(root_node, topology_->units);
    buildTreeShared_(root_node, topology_->shared_units);
}

/**
 * @brief Bind all the ports between different units and set TLBs and preload
 */
auto spike_model::CPUFactory::bindTree(sparta::RootTreeNode* root_node) -> void{
    sparta_assert(topology_);
    bindTree_(root_node, topology_->port_connections);
}

/**
 * @brief Get the list of resources instantiated in this topology
 */
auto spike_model::CPUFactory::getResourceNames() const -> const std::vector<std::string>&{
    return resource_names_;
}
