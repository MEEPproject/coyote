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

#include "PrivateL2Director.hpp"
#include "SharedL2Director.hpp"
#include "utils.hpp"
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

SpikeModel::SpikeModel(sparta::Scheduler & scheduler) :
    sparta::app::Simulation("spike_model", &scheduler)
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

    // Reading general parameters
    const auto& upt = getSimulationConfiguration()->getUnboundParameterTree();
    // meta parameters
    auto    trace                   = upt.get("meta.params.trace").getAs<bool>();
    auto    show_factories          = upt.get("meta.params.show_factories").getAs<bool>();
    auto    arch_topology           = upt.get("meta.params.architecture").getAs<std::string>();
    // architectural parameters
    auto    num_tiles               = upt.get("top.cpu.params.num_tiles").getAs<uint16_t>();
    auto    num_memory_cpus         = upt.get("top.cpu.params.num_memory_cpus").getAs<uint16_t>();
    auto    num_memory_controllers  = upt.get("top.cpu.params.num_memory_controllers").getAs<uint16_t>();
    auto    num_memory_banks        = upt.get("top.cpu.memory_controller0.params.num_banks").getAs<uint64_t>();
    auto    num_l2_banks_in_tile    = upt.get("top.cpu.tile0.params.num_l2_banks").getAs<uint16_t>();
    auto    num_llc_banks_per_mc    = upt.get("top.cpu.memory_cpu0.params.num_llc_banks").getAs<uint16_t>();


    auto cpu_factory = getCPUFactory_();

    // Set the ACME topology that will be built
    cpu_factory->setTopology(arch_topology, num_tiles, num_l2_banks_in_tile, num_memory_cpus, num_llc_banks_per_mc, num_memory_controllers, num_memory_banks, trace);

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
    if(show_factories){
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

std::shared_ptr<spike_model::EventManager> SpikeModel::createRequestManager()
{
    // Reading general parameters
    const auto& upt = getSimulationConfiguration()->getUnboundParameterTree();
    auto num_cores = upt.get("top.cpu.params.num_cores").getAs<uint16_t>();
    auto num_tiles = upt.get("top.cpu.params.num_tiles").getAs<uint16_t>();
    auto x_size = upt.get("top.cpu.params.x_size").getAs<uint16_t>();
    auto y_size = upt.get("top.cpu.params.y_size").getAs<uint16_t>();
    auto num_memory_cpus = upt.get("top.cpu.params.num_memory_cpus").getAs<uint16_t>();
    auto num_memory_controllers = upt.get("top.cpu.params.num_memory_controllers").getAs<uint16_t>();
    auto num_l2_banks_in_tile = upt.get("top.cpu.tile0.params.num_l2_banks").getAs<uint16_t>();
    auto num_llc_banks_per_mc = upt.get("top.cpu.memory_cpu0.params.num_llc_banks").getAs<uint16_t>();

    spike_model::ServicedRequests s;
    std::vector<spike_model::Tile *> tiles;
    for(std::size_t i = 0; i < num_tiles; ++i)
    {
        auto tile_node = getRoot()->getChild(std::string("cpu.tile") +
                sparta::utils::uint32_to_str(i));
        sparta_assert(tile_node != nullptr);

        spike_model::Tile * t=tile_node->getResourceAs<spike_model::Tile>();

        tiles.push_back(t);
    }

    std::shared_ptr<spike_model::EventManager> m=std::make_shared<spike_model::EventManager>(tiles, num_cores/num_tiles);

    //std::shared_ptr<spike_model::PrivateL2Director> p=std::make_shared<spike_model::PrivateL2Director>(tiles, num_cores_per_tile_);
    //std::shared_ptr<spike_model::SharedL2Director> p=std::make_shared<spike_model::SharedL2Director>(tiles, num_cores_per_tile_);
    auto cache_bank_node = getRoot()->getChild(std::string("cpu.tile0.l2_bank0"));
    sparta_assert(cache_bank_node != nullptr);

    spike_model::L2CacheBank * c_b=cache_bank_node->getResourceAs<spike_model::L2CacheBank>();

    uint64_t bank_size=c_b->getSize();
    uint64_t bank_line=c_b->getLineSize();
    uint64_t bank_associativity=c_b->getAssoc();

    for(std::size_t i = 0; i < num_tiles; ++i)
    {
        for(std::size_t j = 0; j < num_l2_banks_in_tile; ++j)
        {
             auto cache_bank_node = getRoot()->getChild(std::string("cpu.tile") +
                                    sparta::utils::uint32_to_str(i) + std::string(".l2_bank") +
                                    sparta::utils::uint32_to_str(j));
             sparta_assert(cache_bank_node != nullptr);
             spike_model::L2CacheBank * c_b=cache_bank_node->getResourceAs<spike_model::L2CacheBank>();
             c_b->set_bank_id(j);
        }
    }

    auto memory_bank_node = getRoot()->getChild(std::string("cpu.memory_controller0.memory_bank0"));
    sparta_assert(memory_bank_node != nullptr);

    spike_model::MemoryBank * m_b=memory_bank_node->getResourceAs<spike_model::MemoryBank>();

    uint64_t num_rows=m_b->getNumRows();
    uint64_t num_cols=m_b->getNumColumns();

    spike_model::AddressMappingPolicy address_mapping=spike_model::AddressMappingPolicy::CLOSE_PAGE;

    for(std::size_t i = 0; i < num_memory_controllers; i++)
    {
        auto mc_node = getRoot()->getChild(std::string("cpu.memory_controller") +
                sparta::utils::uint32_to_str(i));
        sparta_assert(mc_node != nullptr);

        spike_model::MemoryController *mc=mc_node->getResourceAs<spike_model::MemoryController>();

        mc->setup_masks_and_shifts_(num_memory_controllers, num_rows, num_cols, bank_line);
        address_mapping=mc->getAddressMapping();
    }

    m->setServicedRequestsStorage(s);

    uint64_t mc_shift=0;
    uint64_t mc_mask=0;
    switch(address_mapping)
    {
        case spike_model::AddressMappingPolicy::OPEN_PAGE:
            mc_shift=ceil(log2(bank_line));
            mc_mask=utils::nextPowerOf2(num_memory_controllers)-1;
            break;

            
        case spike_model::AddressMappingPolicy::CLOSE_PAGE:
            mc_shift=ceil(log2(bank_line));
            mc_mask=utils::nextPowerOf2(num_memory_controllers)-1;
            break;
    }
    
    uint16_t num_vas_tiles_per_row=x_size-(num_memory_cpus/y_size);
    for(std::size_t i = 0; i < num_tiles; ++i)
    {
        tiles[i]->setRequestManager(m);

        // The corresponding MCPU is the closest in the same row.
        uint16_t row=i/num_vas_tiles_per_row;
        uint16_t corr_mcpu=2*row;
        if(num_tiles>1 && i%num_vas_tiles_per_row>=num_vas_tiles_per_row/2)
        {
            corr_mcpu=corr_mcpu+1;
        }

        tiles[i]->setMemoryInfo(bank_size*num_l2_banks_in_tile, bank_associativity, bank_line, num_l2_banks_in_tile, num_tiles, num_memory_controllers, mc_shift, mc_mask, num_cores, corr_mcpu);
        tiles[i]->getArbiter()->setNumInputs(num_cores/num_tiles, tiles[i]->getL2Banks(), i);
    }

    for(std::size_t i = 0; i < num_memory_cpus; ++i)
    {
        auto tile_node = getRoot()->getChild(std::string("cpu.memory_cpu") +
                sparta::utils::uint32_to_str(i));
        sparta_assert(tile_node != nullptr);

        spike_model::MemoryCPUWrapper *mcpu=tile_node->getResourceAs<spike_model::MemoryCPUWrapper>();
	
        auto llc_node = getRoot()->getChild(std::string("cpu.memory_cpu") + sparta::utils::uint32_to_str(i) + ".llc0");
        sparta_assert(tile_node != nullptr);

        spike_model::L3CacheBank * llc_bank=llc_node->getResourceAs<spike_model::L3CacheBank>();

        //mcpu->setRequestManager(m);
        mcpu->setAddressMappingInfo(mc_shift, mc_mask);
	mcpu->setLLCInfo(llc_bank->getSize()*num_llc_banks_per_mc, llc_bank->getAssoc());
    } 
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
