// <SpikeModeln.cpp> -*- C++ -*-


#include <iostream>

#include "SpikeModel.hpp"
#include "CPUFactory.hpp"

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
#include "simdb/schema/Schema.hpp"
#include "simdb/TableProxy.hpp"
#include "simdb/async/AsyncTaskEval.hpp"
#include "simdb/impl/sqlite/SQLiteConnProxy.hpp"
#include "simdb/impl/hdf5/HDF5ConnProxy.hpp"
#include "simdb/utils/uuids.hpp"

#include "Core.hpp"
#include "PrivateL2Manager.hpp"
#include "SharedL2Manager.hpp"
#include <thread>

double calculateAverageOfInternalCounters(
    const std::vector<const sparta::CounterBase*> & counters)
{
    double agg = 0;
    for (const auto & ctr : counters) {
        agg += ctr->get();
    }
    return agg / counters.size();
}

SpikeModel::SpikeModel(const std::string& topology,
                                   sparta::Scheduler & scheduler,
                                   uint32_t num_cores_per_tile,
                                   uint32_t num_tiles,
                                   uint32_t num_l2_banks,
                                   uint32_t num_memory_controllers,
                                   uint32_t num_memory_banks,
                                   spike_model::AddressMappingPolicy address_mapping_policy,
                                   spike_model::L2SharingPolicy l2_sharing_policy,
                                   spike_model::CacheDataMappingPolicy bank_policy,
                                   spike_model::CacheDataMappingPolicy tile_policy,
                                   std::string cmd,
                                   std::string isa,
                                   bool show_factories,
                                   bool trace) :
    sparta::app::Simulation("spike_model", &scheduler),
    cpu_topology_(topology),
    num_cores_per_tile_(num_cores_per_tile),
    num_tiles_(num_tiles),
    num_l2_banks_(num_l2_banks),
    num_memory_controllers_(num_memory_controllers),
    num_memory_banks_(num_memory_banks),
    address_mapping_policy_(address_mapping_policy),
    l2_sharing_policy_(l2_sharing_policy),
    bank_policy_(bank_policy),
    tile_policy_(tile_policy),
    cmd_(cmd),
    isa_(isa),
    show_factories_(show_factories),
    trace_(trace)
{
    // Set up the CPU Resource Factory to be available through ResourceTreeNode
    getResourceSet()->addResourceFactory<spike_model::CPUFactory>();

    // Initialize example simulation controller
    //controller_.reset(new SpikeModel::ExampleController(this));
    //setSimulationController_(controller_);

    // Register a custom calculation method for 'combining' a context counter's
    // internal counters into one number. In this example simulator, let's just
    // use an averaging function called "avg" which we can then invoke from report
    // definition YAML files.
    sparta::trigger::ContextCounterTrigger::registerContextCounterCalcFunction(
        "avg", &calculateAverageOfInternalCounters);

}


SpikeModel::~SpikeModel()
{
    getRoot()->enterTeardown(); // Allow deletion of nodes without error now
}

//! Get the resource factory needed to build and bind the tree
auto SpikeModel::getCPUFactory_() -> spike_model::CPUFactory*{
    auto sparta_res_factory = getResourceSet()->getResourceFactory("cpu");
    auto cpu_factory = dynamic_cast<spike_model::CPUFactory*>(sparta_res_factory);
    return cpu_factory;
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


void SpikeModel::buildTree_()
{
    // TREE_BUILDING Phase.  See sparta::PhasedObject::TreePhase
    // Register all the custom stat calculation functions with (cycle)histogram nodes

    auto cpu_factory = getCPUFactory_();


    printf("There are %u\n", num_memory_controllers_);

    // Set the cpu topology that will be built
    cpu_factory->setTopology(cpu_topology_, num_tiles_, num_l2_banks_, num_memory_controllers_, num_memory_banks_, trace_);

    // Create a single CPU
    sparta::ResourceTreeNode* cpu_tn = new sparta::ResourceTreeNode(getRoot(),
                                                                "cpu",
                                                                sparta::TreeNode::GROUP_NAME_NONE,
                                                                sparta::TreeNode::GROUP_IDX_NONE,
                                                                "CPU Node",
                                                                cpu_factory);
    to_delete_.emplace_back(cpu_tn);

    // Tell the factory to build the resources now
    cpu_factory->buildTree(getRoot());

    // Print the registered factories
    if(show_factories_){
        std::cout << "Registered factories: \n";
        for(const auto& f : getCPUFactory_()->getResourceNames()){
            std::cout << "\t" << f << std::endl;
        }
    }

}

void SpikeModel::configureTree_()
{


    validateTreeNodeExtensions_();

    // In TREE_CONFIGURING phase
    // Configuration from command line is already applied

    // Read these parameter values to avoid 'unread unbound parameter' exceptions:
    //   top.cpu.core0.dispatch.baz_node.params.baz
    //   top.cpu.core0.fpu.baz_node.params.baz
    //dispatch_baz_->readParams();
    //fpu_baz_->readParams();

    legacy_warmup_report_starter_.reset(new sparta::NotificationSource<uint64_t>(
        getRoot(),
        "all_threads_warmup_instruction_count_retired_re4",
        "Legacy notificiation channel for testing purposes only",
        "all_threads_warmup_instruction_count_retired_re4"));

    getRoot()->REGISTER_FOR_NOTIFICATION(
        onTriggered_, std::string, "sparta_expression_trigger_fired");
    on_triggered_notifier_registered_ = true;
}


void SpikeModel::bindTree_()
{
    // In TREE_FINALIZED phase
    // Tree is finalized. Taps placed. No new nodes at this point
    // Bind appropriate ports

    //Tell the factory to bind all units
    auto cpu_factory = getCPUFactory_();
    cpu_factory->bindTree(getRoot());
}

std::shared_ptr<spike_model::RequestManagerIF> SpikeModel::createRequestManagerIF()
{
    spike_model::ServicedRequests s;
    std::vector<spike_model::Tile *> tiles;
    for(std::size_t i = 0; i < num_tiles_; ++i)
    {
        auto tile_node = getRoot()->getChild(std::string("cpu.tile") +
                sparta::utils::uint32_to_str(i));
        sparta_assert(tile_node != nullptr);

        spike_model::Tile * t=tile_node->getResourceAs<spike_model::Tile>();

        tiles.push_back(t);
    }

    std::shared_ptr<spike_model::RequestManagerIF> m;

    switch(l2_sharing_policy_)
    {
        case spike_model::L2SharingPolicy::TILE_PRIVATE:
            m=std::make_shared<spike_model::PrivateL2Manager>(tiles, num_cores_per_tile_, address_mapping_policy_, bank_policy_);
            break;
        case spike_model::L2SharingPolicy::FULLY_SHARED:
            m=std::make_shared<spike_model::SharedL2Manager>(tiles, num_cores_per_tile_, address_mapping_policy_, bank_policy_, tile_policy_);
            break;
        default:
            std::cout << "Unsupported L2 sharing mode\n";
            exit(-1);
    }

    //std::shared_ptr<spike_model::PrivateL2Manager> p=std::make_shared<spike_model::PrivateL2Manager>(tiles, num_cores_per_tile_);
    //std::shared_ptr<spike_model::SharedL2Manager> p=std::make_shared<spike_model::SharedL2Manager>(tiles, num_cores_per_tile_);
    m->setServicedRequestsStorage(s);
    for(std::size_t i = 0; i < num_tiles_; ++i)
    {
        tiles[i]->setRequestManager(m);
    }
    
    for(std::size_t i = 0; i < num_memory_controllers_; ++i)
    {
        auto tile_node = getRoot()->getChild(std::string("cpu.memory_controller") +
                sparta::utils::uint32_to_str(i));
        sparta_assert(tile_node != nullptr);

        spike_model::MemoryController * mc=tile_node->getResourceAs<spike_model::MemoryController>();
        
        mc->setRequestManager(m);
    }
        
    auto cache_bank_node = getRoot()->getChild(std::string("cpu.tile0.l2_bank0"));
    sparta_assert(cache_bank_node != nullptr);

    spike_model::CacheBank * c_b=cache_bank_node->getResourceAs<spike_model::CacheBank>();

    uint64_t bank_size=c_b->getSize();
    uint64_t bank_line=c_b->getLineSize();
    uint64_t bank_associativity=c_b->getAssoc();
    
    auto memory_bank_node = getRoot()->getChild(std::string("cpu.memory_controller0.memory_bank0"));
    sparta_assert(memory_bank_node != nullptr);

    spike_model::MemoryBank * m_b=memory_bank_node->getResourceAs<spike_model::MemoryBank>();

    uint64_t num_rows=m_b->getNumRows();
    uint64_t num_cols=m_b->getNumColumns();
    
    //ALL THE TILES SHARE A POINTER TO THE SAME REQUEST MANAGER
    m->setMemoryInfo(bank_size*num_l2_banks_*num_tiles_, bank_associativity, bank_line, num_l2_banks_, num_memory_controllers_, num_memory_banks_, num_rows, num_cols);

    return m;
}
    
void SpikeModel::validateTreeNodeExtensions_()
{
}

void SpikeModel::onTriggered_(const std::string & msg)
{
    std::cout << "     [trigger] " << msg << std::endl;
}

spike_model::Logger& SpikeModel::getLogger()
{
    return getCPUFactory_()->getLogger();
}
