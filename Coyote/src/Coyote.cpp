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

// <Coyoten.cpp> -*- C++ -*-


#include <iostream>

#include "Coyote.hpp"
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

Coyote::Coyote(sparta::Scheduler & scheduler) :
    sparta::app::Simulation("coyote", &scheduler)
{
    // Set up the CPU Resource Factory to be available through ResourceTreeNode
    getResourceSet()->addResourceFactory<coyote::CPUFactory>();

    // Initialize example simulation controller
    //controller_.reset(new Coyote::ExampleController(this));
    //setSimulationController_(controller_);

    // Register a custom calculation method for 'combining' a context counter's
    // internal counters into one number. In this example simulator, let's just
    // use an averaging function called "avg" which we can then invoke from report
    // definition YAML files.
    sparta::trigger::ContextCounterTrigger::registerContextCounterCalcFunction(
        "avg", &calculateAverageOfInternalCounters);
}


Coyote::~Coyote()
{
    getRoot()->enterTeardown(); // Allow deletion of nodes without error now
}

//! Get the resource factory needed to build and bind the tree
auto Coyote::getCPUFactory_() -> coyote::CPUFactory*{
    auto sparta_res_factory = getResourceSet()->getResourceFactory("arch");
    auto cpu_factory = dynamic_cast<coyote::CPUFactory*>(sparta_res_factory);
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


void Coyote::buildTree_()
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
    auto    num_tiles               = 0;
    auto    num_memory_cpus         = 0;
    auto    num_memory_controllers  = 0;
    auto    num_memory_banks        = 0;
    auto    num_l2_banks_in_tile    = 0;
    auto    num_llc_banks_per_mc    = 0;

    auto cpu_factory = getCPUFactory_();

    if(arch_topology=="tiled")
    {
        num_tiles               = upt.get("top.arch.params.num_tiles").getAs<uint16_t>();
        num_memory_cpus         = upt.get("top.arch.params.num_memory_cpus").getAs<uint16_t>();
        num_memory_controllers  = upt.get("top.arch.params.num_memory_controllers").getAs<uint16_t>();
        num_memory_banks        = upt.get("top.arch.memory_controller0.params.num_banks").getAs<uint64_t>();
        num_l2_banks_in_tile    = upt.get("top.arch.tile0.params.num_l2_banks").getAs<uint16_t>();
        num_llc_banks_per_mc    = upt.get("top.arch.memory_cpu0.params.num_llc_banks").getAs<uint16_t>();
    }
    else if(arch_topology=="memory_controller_unit_test")
    {
        num_memory_controllers=1;
        num_memory_banks        = upt.get("top.arch.memory_controller.params.num_banks").getAs<uint64_t>();
    }

    // Set the ACME topology that will be built
    cpu_factory->setTopology(arch_topology, num_tiles, num_l2_banks_in_tile, num_memory_cpus, num_llc_banks_per_mc, num_memory_controllers, num_memory_banks, trace);

    // Create a single CPU
    sparta::ResourceTreeNode* cpu_tn = new sparta::ResourceTreeNode(getRoot(),
                                                                "arch",
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

void Coyote::configureTree_()
{
    validateTreeNodeExtensions_();

    // In TREE_CONFIGURING phase
    // Configuration from command line is already applied

    // Read these parameter values to avoid 'unread unbound parameter' exceptions:
    //   top.arch.core0.dispatch.baz_node.params.baz
    //   top.arch.core0.fpu.baz_node.params.baz
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


void Coyote::bindTree_()
{
    // In TREE_FINALIZED phase
    // Tree is finalized. Taps placed. No new nodes at this point
    // Bind appropriate ports

    //Tell the factory to bind all units
    auto cpu_factory = getCPUFactory_();
    cpu_factory->bindTree(getRoot());
    
    const auto& upt = getSimulationConfiguration()->getUnboundParameterTree();
    auto    arch_topology           = upt.get("meta.params.architecture").getAs<std::string>();    
}

std::shared_ptr<coyote::FullSystemSimulationEventManager> Coyote::createRequestManager()
{
    //MANY THINGS HERE COUYLD BE MOVED TO BIND IN CPUFactory, which should be refactored as well...

    // Reading general parameters
    const auto& upt = getSimulationConfiguration()->getUnboundParameterTree();
    auto num_cores = upt.get("top.arch.params.num_cores").getAs<uint16_t>();
    auto num_tiles = upt.get("top.arch.params.num_tiles").getAs<uint16_t>();
    auto x_size = upt.get("top.arch.params.x_size").getAs<uint16_t>();
    auto y_size = upt.get("top.arch.params.y_size").getAs<uint16_t>();
    auto num_memory_cpus = upt.get("top.arch.params.num_memory_cpus").getAs<uint16_t>();
    auto num_memory_controllers = upt.get("top.arch.params.num_memory_controllers").getAs<uint16_t>();
    auto num_l2_banks_in_tile = upt.get("top.arch.tile0.params.num_l2_banks").getAs<uint16_t>();
    auto num_llc_banks_per_mc = upt.get("top.arch.memory_cpu0.params.num_llc_banks").getAs<uint16_t>();
    auto num_mem_banks_per_mc = upt.get("top.arch.memory_controller0.params.num_banks").getAs<uint64_t>();
    auto unused_lsbs              = upt.get("top.arch.memory_controller0.params.unused_lsbs").getAs<uint8_t>();

    coyote::ServicedRequests s;
    std::vector<coyote::Tile *> tiles;
    for(std::size_t i = 0; i < num_tiles; ++i)
    {
        auto tile_node = getRoot()->getChild(std::string("arch.tile") +
                sparta::utils::uint32_to_str(i));
        sparta_assert(tile_node != nullptr);

        coyote::Tile * t=tile_node->getResourceAs<coyote::Tile>();

        tiles.push_back(t);
    }

    std::shared_ptr<coyote::FullSystemSimulationEventManager> m=std::make_shared<coyote::FullSystemSimulationEventManager>(tiles, num_cores/num_tiles);

    //std::shared_ptr<coyote::PrivateL2Director> p=std::make_shared<coyote::PrivateL2Director>(tiles, num_cores_per_tile_);
    //std::shared_ptr<coyote::SharedL2Director> p=std::make_shared<coyote::SharedL2Director>(tiles, num_cores_per_tile_);
    auto cache_bank_node = getRoot()->getChild(std::string("arch.tile0.l2_bank0"));
    sparta_assert(cache_bank_node != nullptr);

    coyote::L2CacheBank * c_b=cache_bank_node->getResourceAs<coyote::L2CacheBank>();

    uint64_t bank_size=c_b->getSize();
    uint64_t bank_line=c_b->getLineSize();
    uint64_t bank_associativity=c_b->getAssoc();

    for(std::size_t i = 0; i < num_tiles; ++i)
    {
        for(std::size_t j = 0; j < num_l2_banks_in_tile; ++j)
        {
             auto cache_bank_node = getRoot()->getChild(std::string("arch.tile") +
                                    sparta::utils::uint32_to_str(i) + std::string(".l2_bank") +
                                    sparta::utils::uint32_to_str(j));
             sparta_assert(cache_bank_node != nullptr);
             coyote::L2CacheBank * c_b=cache_bank_node->getResourceAs<coyote::L2CacheBank>();
             c_b->set_bank_id(j);
        }
    }

    auto memory_bank_node = getRoot()->getChild(std::string("arch.memory_controller0.memory_bank0"));
    sparta_assert(memory_bank_node != nullptr);

    coyote::MemoryBank * m_b=memory_bank_node->getResourceAs<coyote::MemoryBank>();

    uint64_t num_rows=m_b->getNumRows();
    uint64_t num_cols=m_b->getNumColumns();

    coyote::AddressMappingPolicy address_mapping=coyote::AddressMappingPolicy::CLOSE_PAGE;

    for(std::size_t i = 0; i < num_memory_controllers; i++)
    {
        auto mc_node = getRoot()->getChild(std::string("arch.memory_controller") +
                sparta::utils::uint32_to_str(i));
        sparta_assert(mc_node != nullptr);

        coyote::MemoryController *mc=mc_node->getResourceAs<coyote::MemoryController>();

        mc->setup_masks_and_shifts_(num_memory_controllers, num_rows, num_cols);
        address_mapping=mc->getAddressMapping();
    }

    m->setServicedRequestsStorage(s);

    uint64_t mc_shift=0;
    uint64_t mc_mask=0;
    switch(address_mapping)
    {
        case coyote::AddressMappingPolicy::OPEN_PAGE:
            mc_shift=ceil(log2(bank_line));
            mc_mask=utils::nextPowerOf2(num_memory_controllers)-1;
            break; 
        case coyote::AddressMappingPolicy::CLOSE_PAGE:
            mc_shift=ceil(log2(bank_line));
            mc_mask=utils::nextPowerOf2(num_memory_controllers)-1;
            break;
        case coyote::AddressMappingPolicy::ROW_BANK_COLUMN_BANK_GROUP_INTERLEAVE:
            mc_shift=log2(num_mem_banks_per_mc)+log2(num_rows)+log2(num_cols)+unused_lsbs;
            mc_mask=utils::nextPowerOf2(num_memory_controllers)-1;
            break;
        case coyote::AddressMappingPolicy::ROW_COLUMN_BANK:
            mc_shift=log2(num_mem_banks_per_mc)+log2(num_rows)+log2(num_cols)+unused_lsbs;
            mc_mask=utils::nextPowerOf2(num_memory_controllers)-1;
            break;
        case coyote::AddressMappingPolicy::BANK_ROW_COLUMN:
            mc_shift=log2(num_mem_banks_per_mc)+log2(num_rows)+log2(num_cols)+unused_lsbs;
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
        auto tile_node = getRoot()->getChild(std::string("arch.memory_cpu") +
                sparta::utils::uint32_to_str(i));
        sparta_assert(tile_node != nullptr);

        coyote::MemoryCPUWrapper *mcpu=tile_node->getResourceAs<coyote::MemoryCPUWrapper>();
	
        auto llc_node = getRoot()->getChild(std::string("arch.memory_cpu") + sparta::utils::uint32_to_str(i) + ".llc0");
        sparta_assert(tile_node != nullptr);

        coyote::L3CacheBank * llc_bank=llc_node->getResourceAs<coyote::L3CacheBank>();

        //mcpu->setRequestManager(m);
        mcpu->setAddressMappingInfo(mc_shift, mc_mask);
	mcpu->setLLCInfo(llc_bank->getSize()*num_llc_banks_per_mc, llc_bank->getAssoc());
    } 
    return m;
}

void Coyote::validateTreeNodeExtensions_()
{
}

void Coyote::onTriggered_(const std::string & msg)
{
    std::cout << "     [trigger] " << msg << std::endl;
}

coyote::Logger * Coyote::getLogger()
{
    return getCPUFactory_()->getLogger();
}

double Coyote::getAvgArbiterLatency()
{
    const auto& upt=getSimulationConfiguration()->getUnboundParameterTree();
    auto num_tiles=upt.get("top.arch.params.num_tiles").getAs<uint16_t>();
    double res=0;

    double num_local_requests=0;
    double num_remote_requests=0;
    for(std::size_t i = 0; i < num_tiles; ++i)
    {
        sparta::CounterBase* c_local=getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.tile")+sparta::utils::uint32_to_str(i)+std::string(".stats.requests_from_local_cores"));
        sparta::CounterBase* c_remote=getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.tile")+sparta::utils::uint32_to_str(i)+std::string(".stats.requests_from_remote_cores"));
        sparta_assert(c_local!=nullptr && c_remote!=nullptr);

        num_local_requests+=c_local->get();
        num_remote_requests+=c_remote->get();
    }
    uint64_t total_requests=num_local_requests+num_remote_requests;
    res=total_requests;

    double avg_latency=0;
    for(std::size_t i = 0; i < num_tiles; ++i)
    {
        sparta::CounterBase* c_time=getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.tile")+sparta::utils::uint32_to_str(i)+std::string(".arbiter.stats.total_time_spent_by_messages"));
        sparta::CounterBase* c_mes=getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.tile")+sparta::utils::uint32_to_str(i)+std::string(".arbiter.stats.messages"));

        avg_latency+=(double)c_time->get()/c_mes->get();
    }
    avg_latency=avg_latency/num_tiles;

    res=((num_local_requests/total_requests)*4*avg_latency)+((num_remote_requests/total_requests)*5*avg_latency);

    return res;
}

double Coyote::getAvgL2Latency()
{
    const auto& upt=getSimulationConfiguration()->getUnboundParameterTree();
    auto num_tiles=upt.get("top.arch.params.num_tiles").getAs<uint16_t>();
    auto num_l2_banks=upt.get("top.arch.tile0.params.num_l2_banks").getAs<uint16_t>();
    
    double count_requests=0;
    double count_times=0;
    for(std::size_t i = 0; i < num_tiles; ++i)
    {
        for(std::size_t j = 0; j < num_l2_banks; ++j)
        {
            sparta::CounterBase* c_time = getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.tile") +
                                    sparta::utils::uint32_to_str(i) + std::string(".l2_bank") +
                                    sparta::utils::uint32_to_str(j) + std::string(".stats.total_time_spent_by_requests"));
            sparta::StatisticDef* c_reads = getRoot()->getChildAs<sparta::StatisticDef>(std::string("arch.tile") +
                                    sparta::utils::uint32_to_str(i) + std::string(".l2_bank") +
                                    sparta::utils::uint32_to_str(j) + std::string(".stats.overall_reads"));
            sparta::StatisticDef* c_writes = getRoot()->getChildAs<sparta::StatisticDef>(std::string("arch.tile") +
                                    sparta::utils::uint32_to_str(i) + std::string(".l2_bank") +
                                    sparta::utils::uint32_to_str(j) + std::string(".stats.overall_writes"));
            sparta::StatisticInstance reads(c_reads);
            sparta::StatisticInstance writes(c_writes);
            count_requests+=reads.getValue()+writes.getValue();
            count_times+=c_time->get();

        }
    }
    return count_times/count_requests;
}

// Returns the avg. RTT
double Coyote::getAvgNoCLatency()
{
    const auto& upt=getSimulationConfiguration()->getUnboundParameterTree();
    auto noc_model=upt.get("top.arch.noc.params.noc_model").getAs<std::string>();
    
    auto num_tiles=upt.get("top.arch.params.num_tiles").getAs<uint16_t>();
    
    double num_remote_requests=0; // Is used in float operations
    double num_local_requests=0; // Is used in float operations
    
    for(std::size_t i = 0; i < num_tiles; ++i)
    {
        sparta::CounterBase* c_local=getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.tile")+sparta::utils::uint32_to_str(i)+std::string(".stats.requests_from_local_cores"));
        sparta::CounterBase* c_remote=getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.tile")+sparta::utils::uint32_to_str(i)+std::string(".stats.requests_from_remote_cores"));
        sparta_assert(c_local!=nullptr && c_remote!=nullptr);

        num_local_requests+=c_local->get();
        num_remote_requests+=c_remote->get();
    }
    uint64_t total_requests=num_local_requests+num_remote_requests;

    double res=0;
    if (noc_model == "functional")
    {
        double avg_latency=upt.get("top.arch.noc.params.packet_latency").getAs<std::uint64_t>();
        res=((num_local_requests/total_requests)*2*avg_latency)+((num_remote_requests/total_requests)*4*avg_latency); 
        // Local is *2 because there are 1 packet with request and 1 with reply and remote is *4 because *2 is to reach memory and *2 to reach its home tile
    }
    else if (noc_model == "simple")
    {
        auto noc = getRoot()->getChild(std::string("arch.noc"))->getResourceAs<coyote::SimpleNoC>();
        std::string remote_l2_request_noc = noc->getNetworkName(coyote::NoC::getNetworkForMessage(coyote::NoCMessageType::REMOTE_L2_REQUEST));
        std::string remote_l2_ack_noc     = noc->getNetworkName(coyote::NoC::getNetworkForMessage(coyote::NoCMessageType::REMOTE_L2_ACK));
        std::string mem_request_noc       = noc->getNetworkName(coyote::NoC::getNetworkForMessage(coyote::NoCMessageType::MEMORY_REQUEST_LOAD));
        std::string mem_ack_noc           = noc->getNetworkName(coyote::NoC::getNetworkForMessage(coyote::NoCMessageType::MEMORY_ACK));

        double hop_latency=upt.get("top.arch.noc.params.latency_per_hop").getAs<std::uint16_t>();
        double avg_hop_count_local = 1.0 * getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.hop_count_" + mem_request_noc))->get()/getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.sent_packets_" + mem_request_noc))->get()
                                   + 1.0 * getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.hop_count_" + mem_ack_noc))->get()/getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.sent_packets_" + mem_ack_noc))->get();
        double avg_hop_count_remote = avg_hop_count_local
                                    + 1.0 * getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.hop_count_" + remote_l2_request_noc))->get()/getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.sent_packets_" + remote_l2_request_noc))->get()
                                    + 1.0 * getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.hop_count_" + remote_l2_ack_noc))->get()/getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.sent_packets_" + remote_l2_ack_noc))->get();
        res=((num_local_requests/total_requests)*avg_hop_count_local*hop_latency)+((num_remote_requests/total_requests)*avg_hop_count_remote*hop_latency);
    }
    else if (noc_model == "detailed")
    {
        double avg_latency_local = 1.0 * getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.packet_latency_MEMORY_REQUEST_LOAD"))->get()/getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.num_MEMORY_REQUEST_LOAD"))->get()
                                 + 1.0 * getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.packet_latency_MEMORY_ACK"))->get()/getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.num_MEMORY_ACK"))->get();
        double avg_latency_remote = avg_latency_local
                                  + 1.0 * getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.packet_latency_REMOTE_L2_REQUEST"))->get()/getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.num_REMOTE_L2_REQUEST"))->get()
                                  + 1.0 * getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.packet_latency_REMOTE_L2_ACK"))->get()/getRoot()->getChildAs<sparta::Counter>(std::string("arch.noc.stats.num_REMOTE_L2_ACK"))->get();
        res=((num_local_requests/total_requests)*avg_latency_local) + ((num_remote_requests/total_requests)*avg_latency_remote);
    }
    return res;
}

double Coyote::getAvgLLCLatency()
{
    const auto& upt=getSimulationConfiguration()->getUnboundParameterTree();
    auto num_memory_controllers=upt.get("top.arch.params.num_memory_controllers").getAs<uint16_t>();
    auto num_llc_banks=upt.get("top.arch.memory_cpu0.params.num_llc_banks").getAs<uint16_t>();
    
    double count_requests=0;
    double count_times=0;
    for(std::size_t i = 0; i < num_memory_controllers; ++i)
    {
        for(std::size_t j = 0; j < num_llc_banks; ++j)
        {
            sparta::CounterBase* c_time = getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.memory_cpu") +
                                    sparta::utils::uint32_to_str(i) + std::string(".llc") +
                                    sparta::utils::uint32_to_str(j) + std::string(".stats.total_time_spent_by_requests"));
            sparta::StatisticDef* c_reads = getRoot()->getChildAs<sparta::StatisticDef>(std::string("arch.memory_cpu") +
                                    sparta::utils::uint32_to_str(i) + std::string(".llc") +
                                    sparta::utils::uint32_to_str(j) + std::string(".stats.overall_reads"));
            sparta::StatisticDef* c_writes = getRoot()->getChildAs<sparta::StatisticDef>(std::string("arch.memory_cpu") +
                                    sparta::utils::uint32_to_str(i) + std::string(".llc") +
                                    sparta::utils::uint32_to_str(j) + std::string(".stats.overall_writes"));
            sparta::StatisticInstance reads(c_reads);
            sparta::StatisticInstance writes(c_writes);
            count_requests+=reads.getValue()+writes.getValue();
            count_times+=c_time->get();

        }
    }
    return count_times/count_requests;

}

double Coyote::getAvgMemoryControllerLatency()
{
    const auto& upt=getSimulationConfiguration()->getUnboundParameterTree();
    auto num_memory_controllers=upt.get("top.arch.params.num_memory_controllers").getAs<uint16_t>();
    
    double count_reads=0;
    double count_writes=0;
    double count_time_writes=0;
    double count_time_reads=0;
    for(std::size_t i = 0; i < num_memory_controllers; ++i)
    {
        sparta::CounterBase* c_time_r = getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.memory_controller") +
                                sparta::utils::uint32_to_str(i) + std::string(".stats.total_time_spent_by_load_requests"));
        sparta::CounterBase* c_time_w = getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.memory_controller") +
                                sparta::utils::uint32_to_str(i) + std::string(".stats.total_time_spent_by_store_requests"));
        sparta::CounterBase* c_reads = getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.memory_controller") +
                                sparta::utils::uint32_to_str(i) + std::string(".stats.load_requests"));
        sparta::CounterBase* c_writes = getRoot()->getChildAs<sparta::CounterBase>(std::string("arch.memory_controller") +
                                sparta::utils::uint32_to_str(i) + std::string(".stats.store_requests"));
        count_reads+=c_reads->get();
        count_writes+=c_writes->get();
        count_time_reads+=c_time_r->get();
        count_time_writes+=c_time_w->get();
    }
    double avg_reads=count_time_reads/count_reads;
    double avg_writes=count_time_writes/count_writes;

    double total_requests=count_reads+count_writes;

    return (avg_reads*count_reads/total_requests) + (avg_writes*count_writes/total_requests);
}
