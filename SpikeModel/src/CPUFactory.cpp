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
                                           const uint32_t num_tiles,
                                            const uint32_t num_banks_per_tile,
                                             const uint32_t num_memory_controllers,
                                              const uint32_t num_memory_banks,
                                             const bool trace) -> void{
    sparta_assert(!topology_);
    topology_.reset(spike_model::CPUTopology::allocateTopology(topology));
    topology_->setName(topology);
    topology_->setNumTiles(num_tiles);
    topology_->setNumL2BanksPerTile(num_banks_per_tile);
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
auto spike_model::CPUFactory::buildTree_(sparta::RootTreeNode* root_node,
                                          const std::vector<spike_model::CPUTopology::UnitInfo>& units) -> void{
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
    }
}

auto spike_model::CPUFactory::buildTreeShared_(sparta::RootTreeNode* root_node,
                                          const std::vector<spike_model::CPUTopology::UnitInfo>& units) -> void{
    std::string parent_name, human_name, node_name, replace_with, og_name;
    for(const auto& unit : units)
    {
        for(std::size_t num_of_l2_banks = 0; num_of_l2_banks < topology_->num_banks_per_tile; ++num_of_l2_banks)
        {
            og_name=unit.name;
            parent_name = unit.parent_name;
            node_name = unit.name;
            human_name = unit.human_name;
            replace_with = std::to_string(num_of_l2_banks);
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

    //CacheBank * bank=root_node->getChild(std::string("cpu.tile0.l2_bank0"))->getResourceAs<spike_model::CacheBank>();
    

    if(topology_->trace)
    {
        noc->setLogger(topology_->logger);
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
            auto mc_node = root_node->getChild(std::string("cpu.memory_controller") +
                    sparta::utils::uint32_to_str(num_of_memory_controllers));
            sparta_assert(mc_node != nullptr);
            MemoryController * mc=mc_node->getResourceAs<spike_model::MemoryController>();
    
            for(std::size_t num_of_memory_banks = 0; num_of_memory_banks < topology_->num_memory_banks; ++num_of_memory_banks)
            {
                auto bank_node = root_node->getChild(std::string("cpu.memory_controller") +
                        sparta::utils::uint32_to_str(num_of_memory_controllers) + std::string(".memory_bank") + sparta::utils::uint32_to_str(num_of_memory_banks));
                sparta_assert(bank_node != nullptr);
                MemoryBank * b=bank_node->getResourceAs<spike_model::MemoryBank>();
                mc->addBank_(b);
                b->setMemoryController(mc);
            }


            for(const auto& port : ports)
            {
                bool bind=false;
                out_port_name = port.output_port_name;
                in_port_name = port.input_port_name;
                replace_with = std::to_string(num_of_memory_controllers);
                if (out_port_name.find("memory_controller&") != std::string::npos) 
                {
                    replace(out_port_name, to_replace_memory_controllers_, replace_with);
                    bind=true;
                }
                
                if (in_port_name.find("memory_controller&") != std::string::npos)
                {
                    replace(in_port_name, to_replace_memory_controllers_, replace_with);
                    bind=true;
                } 
                    
                if(bind)
                {
                    std::cout << "Binding " << out_port_name << " and " << in_port_name << "\n";
                    sparta::bind(root_node->getChildAs<sparta::Port>(out_port_name),
                            root_node->getChildAs<sparta::Port>(in_port_name));
                }
            }
    }

    
    for(std::size_t num_of_tiles = 0; num_of_tiles < topology_->num_tiles; ++num_of_tiles){
            auto core_node = root_node->getChild(std::string("cpu.tile") +
                sparta::utils::uint32_to_str(num_of_tiles));
            sparta_assert(core_node != nullptr);

            Tile * tile=core_node->getResourceAs<spike_model::Tile>();

            tile->setId(num_of_tiles);
            
            /*if(topology_->trace)
            {
                core->setLogger(topology_->logger);
            }*/
    }
    
    if(topology_->trace)
    {
        for(std::size_t num_of_tiles = 0; num_of_tiles < topology_->num_tiles; ++num_of_tiles)
        {
            auto tile_node = root_node->getChild(std::string("cpu.tile") +
                    sparta::utils::uint32_to_str(num_of_tiles));
            sparta_assert(tile_node != nullptr);

            Tile * tile=tile_node->getResourceAs<spike_model::Tile>();

            tile->setLogger(topology_->logger);

            for(std::size_t num_of_l2_banks = 0; num_of_l2_banks < topology_->num_banks_per_tile; ++num_of_l2_banks){
                auto bank_node = root_node->getChild(std::string("cpu.tile") + sparta::utils::uint32_to_str(num_of_tiles) +
                        std::string(".l2_bank") + sparta::utils::uint32_to_str(num_of_l2_banks));
                sparta_assert(bank_node != nullptr);

                CacheBank * bank=bank_node->getResourceAs<spike_model::CacheBank>();

                bank->setLogger(topology_->logger);
            }
        }

        for(std::size_t num_of_memory_controllers = 0; num_of_memory_controllers < topology_->num_memory_controllers; ++num_of_memory_controllers)
        {
            auto mc_node = root_node->getChild(std::string("cpu.memory_controller") +
                    sparta::utils::uint32_to_str(num_of_memory_controllers));
            sparta_assert(mc_node != nullptr);

            MemoryController * mc=mc_node->getResourceAs<spike_model::MemoryController>();

            mc->setLogger(topology_->logger);
        }
    }

    printf("HOLA\n");
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

spike_model::Logger& spike_model::CPUFactory::getLogger()
{
    return topology_->logger;
}
