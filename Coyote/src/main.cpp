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

// <main.cpp> -*- C++ -*-

#include "Coyote.hpp" // Core model skeleton simulator
#include "Request.hpp"
#include "FullSystemSimulationEventManager.hpp"
#include "spike_wrapper.h"
#include "L2SharingPolicy.hpp"
#include "CacheDataMappingPolicy.hpp"
#include "ExecutionDrivenSimulationOrchestrator.hpp"
#include "TraceDrivenSimulationOrchestrator.hpp"
#include "NoC/NoC.hpp"
#include "SimulationEntryPoint.hpp"

#include "sparta/parsers/ConfigEmitterYAML.hpp"
#include "sparta/app/CommandLineSimulator.hpp"
#include "sparta/app/MultiDetailOptions.hpp"
#include "sparta/sparta.hpp"
#include "sparta/utils/SpartaAssert.hpp"

#include <iostream>
#include <string> 
#include <chrono>
#include "stdlib.h"

// User-friendly usage that correspond with sparta::app::CommandLineSimulator
// options
const char USAGE[] =
    "Usage:\n"
    "    [--show-tree] [--show-dag]\n"
    "    [-p PATTERN VAL] [-c FILENAME]\n"
    "    [--node-config-file PATTERN FILENAME]\n"
    "    [-l PATTERN CATEGORY DEST]\n"
    "    [-h]\n"
    "\n";

constexpr char VERSION_VARNAME[] = "version"; // Name of option to show version
constexpr char DATA_FILE_VARNAME[] = "data-file"; // Name of data file given at the end of command line
constexpr char DATA_FILE_OPTIONS[] = "data-file"; // Data file options, which are flag independent

void setupTiledSimulation(sparta::app::CommandLineSimulator& cls)
{
    const auto upt = cls.getSimulationConfiguration().getUnboundParameterTree();

    auto num_memory_cpus        = upt.get("top.arch.params.num_memory_cpus").getAs<uint16_t>();
    auto noc_model              = upt.get("top.arch.noc.params.noc_model").getAs<std::string>();
    auto bank_policy            = upt.get("top.arch.tile0.params.bank_policy").getAs<std::string>();
    auto tile_policy            = upt.get("top.arch.tile0.params.tile_policy").getAs<std::string>();
    auto sharing                = upt.get("top.arch.tile0.params.l2_sharing_mode").getAs<std::string>();
    auto num_banks              = upt.get("top.arch.tile0.params.num_l2_banks").getAs<uint16_t>();
    auto x_size                 = upt.get("top.arch.params.x_size").getAs<uint16_t>();
    auto y_size                 = upt.get("top.arch.params.y_size").getAs<uint16_t>();
    auto mcpus_indices          = upt.get("top.arch.params.mcpus_indices").getAs<std::string>();
    auto num_tiles              = upt.get("top.arch.params.num_tiles").getAs<uint16_t>();
    auto icache_config          = upt.get("top.arch.params.icache_config").getAs<std::string>();
    auto dcache_config          = upt.get("top.arch.params.dcache_config").getAs<std::string>();
    auto mcpu_line_size         = upt.get("top.arch.memory_cpu0.params.line_size").getAs<uint64_t>();
    auto l2bank_line_size       = upt.get("top.arch.tile0.l2_bank0.params.line_size").getAs<uint64_t>();

    // Copy parameters shared by multiple units
    std::string num_tiles_p ("top.arch.noc.params.num_tiles");
    cls.getSimulationConfiguration().processParameter(num_tiles_p, sparta::utils::uint32_to_str(num_tiles));
    std::string num_mcpus_p ("top.arch.noc.params.num_memory_cpus");
    cls.getSimulationConfiguration().processParameter(num_mcpus_p, sparta::utils::uint32_to_str(num_memory_cpus));
    std::string num_mcs_p ("top.arch.params.num_memory_controllers");
    cls.getSimulationConfiguration().processParameter(num_mcs_p, sparta::utils::uint32_to_str(num_memory_cpus));
    std::string x_size_p ("top.arch.noc.params.x_size");
    cls.getSimulationConfiguration().processParameter(x_size_p, sparta::utils::uint32_to_str(x_size));
    std::string y_size_p ("top.arch.noc.params.y_size");
    cls.getSimulationConfiguration().processParameter(y_size_p, sparta::utils::uint32_to_str(y_size));
    std::string mcpus_indices_p ("top.arch.noc.params.mcpus_indices");
    cls.getSimulationConfiguration().processParameter(mcpus_indices_p, mcpus_indices);

    uint32_t bank_and_tile_bits=1;

    if(bank_policy=="set_interleaving")
    {
        bank_and_tile_bits *= num_banks;
    }
    if(tile_policy=="set_interleaving" && sharing=="fully_shared")
    {
        bank_and_tile_bits *= num_tiles;
    }
    cls.getSimulationConfiguration().processParameter("top.arch.tile*.l2_bank*.params.bank_and_tile_offset", sparta::utils::uint32_to_str(bank_and_tile_bits));


    // Some general parameters checks
    std::string dcache_line_size=dcache_config.substr(dcache_config.rfind(":") + 1);
    std::string icache_line_size=icache_config.substr(icache_config.rfind(":") + 1);
    sparta_assert((l2bank_line_size == mcpu_line_size)            &&
                  (l2bank_line_size == stoul(dcache_line_size)) &&
                  (l2bank_line_size == stoul(icache_line_size)),
        "The L1$D, L1$I, l2_bank and MCPU line size parameters must have the same value. Current values are:" <<
        "\ntop.arch.tile*.l2_bank*.params.line_size: " << l2bank_line_size <<
        "\ntop.arch.memory_cpu*.params.line_size: " << mcpu_line_size <<
        "\nL1$D line size: " << dcache_line_size << " because top.arch.params.dcache_config is: " << dcache_config <<
        "\nL1$I line size: " << icache_line_size << " because top.arch.params.icache_config is: " << icache_config);            
} 

std::shared_ptr<SimulationOrchestrator> createTraceDrivenOrchestrator(sparta::app::CommandLineSimulator& cls, std::shared_ptr<Coyote>& sim, coyote::NoC* noc)
{
    const auto upt = cls.getSimulationConfiguration().getUnboundParameterTree();

    auto architecture           = upt.get("meta.params.architecture").getAs<std::string>();
    auto trace                  = upt.get("meta.params.trace").getAs<bool>();
    auto cmd                    = upt.get("meta.params.cmd").getAs<std::string>();

    coyote::SimulationEntryPoint * entry_point=NULL;
    if(architecture=="tiled")
    {
        entry_point=sim->createRequestManager().get();
    }
    else if(architecture=="l2_unit_test")
    {
        printf("Well, well...\n");
        entry_point=sim->getRoot()->getChild(std::string("arch.l2_bank"))->getResourceAs<coyote::SimulationEntryPoint>();
    }
    else if(architecture=="memory_controller_unit_test")
    {
        entry_point=sim->getRoot()->getChild(std::string("arch.memory_controller"))->getResourceAs<coyote::SimulationEntryPoint>();
    }
    else
    {
        sparta_assert(false, "Unsupported architecture " << architecture << " for trace driven simulation.");
    }
    return std::make_shared<TraceDrivenSimulationOrchestrator>(cmd, sim, entry_point, trace, noc);
}

std::shared_ptr<SimulationOrchestrator> createExecutionDrivenOrchestrator(sparta::app::CommandLineSimulator& cls, std::shared_ptr<Coyote>& sim, coyote::NoC* noc)
{
    const auto upt = cls.getSimulationConfiguration().getUnboundParameterTree();
    
    auto architecture           = upt.get("meta.params.architecture").getAs<std::string>();
    sparta_assert(architecture=="tiled", "Unsupported architecture " << architecture << " for execution driven simulation.");

    auto fast_cache             = upt.get("meta.params.fast_cache").getAs<bool>();
    auto cmd                    = upt.get("meta.params.cmd").getAs<std::string>();
    auto enable_smart_mcpu      = upt.get("top.arch.memory_cpu0.params.enable_smart_mcpu").getAs<bool>();
    auto vector_bypass_l1       = upt.get("meta.params.vector_bypass_l1").getAs<bool>();
    auto vector_bypass_l2       = upt.get("meta.params.vector_bypass_l2").getAs<bool>();
    auto l1_writeback           = upt.get("meta.params.l1_writeback").getAs<bool>();
    auto trace                  = upt.get("meta.params.trace").getAs<bool>();
    auto events_of_interest     = upt.get("meta.params.events_to_trace").getAs<std::string>();
    // architectural parameters
    auto isa                    = upt.get("top.arch.params.isa").getAs<std::string>();
    auto num_tiles              = upt.get("top.arch.params.num_tiles").getAs<uint16_t>();
    auto num_cores              = upt.get("top.arch.params.num_cores").getAs<uint16_t>();
    auto num_threads_per_core   = upt.get("top.arch.params.num_threads_per_core").getAs<uint16_t>();
    auto thread_switch_latency  = upt.get("top.arch.params.thread_switch_latency").getAs<uint16_t>();
    auto num_mshrs_per_core     = upt.get("top.arch.params.num_mshrs_per_core").getAs<uint16_t>();
    auto varch                  = upt.get("top.arch.params.varch").getAs<std::string>();
    auto lanes_per_vpu          = upt.get("top.arch.params.lanes_per_vpu").getAs<std::uint16_t>();
    auto num_banks              = upt.get("top.arch.tile0.params.num_l2_banks").getAs<uint16_t>();
    auto l2bank_size            = upt.get("top.arch.tile0.l2_bank0.params.size_kb").getAs<uint64_t>();
    auto lvrf_ways              = upt.get("top.arch.tile0.l2_bank0.params.lvrf_ways").getAs<uint64_t>();
    auto icache_config          = upt.get("top.arch.params.icache_config").getAs<std::string>();
    auto dcache_config          = upt.get("top.arch.params.dcache_config").getAs<std::string>();

    sparta_assert(!enable_smart_mcpu || lvrf_ways>0, "At least 1 way in the L2 needs to be used for the LVRF if the MCPU is enabled! Please check parameter lvrf_ways.");

    std::shared_ptr<coyote::FullSystemSimulationEventManager> request_manager=sim->createRequestManager();

    size_t scratchpad_size = ((num_banks * l2bank_size * 1024 * 8) / (num_cores / num_tiles)) / 32;
 
    std::shared_ptr<coyote::SpikeWrapper> spike=std::make_shared<coyote::SpikeWrapper>(
        std::to_string(num_cores),              // Number of cores to simulate
        std::to_string(num_threads_per_core),   // Number of cores in each core
        icache_config,                          // L1$I cache configuration
        dcache_config,                          // L1$D cache configuration
        isa,                                    // RISC-V ISA
        cmd,                                    // Application to execute
        varch,                                  // RISC-V Vector uArch string
        fast_cache,                             // Use a fast L1 cache model instead of the default spike cache
        enable_smart_mcpu,                      // Enable smart mcpu
        
        vector_bypass_l1,
        vector_bypass_l2,
        l1_writeback,
        lanes_per_vpu,
        scratchpad_size);

    if(trace)
    {
        if(events_of_interest=="[any]" || events_of_interest.find("instruction_log")!=std::string::npos)
        {
            coyote::Logger * l=sim->getLogger();
            uint64_t start=0;
            uint64_t end=std::numeric_limits<uint64_t>::max();
            if(upt.hasValue("meta.params.trace_start_tick") && upt.hasValue("meta.params.trace_end_tick"))
            {
                start=upt.get("meta.params.trace_start_tick").getAs<uint64_t>();
                end=upt.get("meta.params.trace_end_tick").getAs<uint64_t>();
            }
            spike->setInstructionLogFile(l->getFile(), start, end);
        }
        else
        {
            std::cout << "Instruction logging requires tracing to be enabled. Running in normal mode\n";
        }
    }
    return std::make_shared<ExecutionDrivenSimulationOrchestrator>(spike, sim, request_manager, num_cores, num_threads_per_core,
                thread_switch_latency, num_mshrs_per_core, trace, l1_writeback, noc);
}
                
int main(int argc, char **argv)
{

    sparta::app::DefaultValues DEFAULTS;
    DEFAULTS.auto_summary_default = "on";
    // try/catch block to ensure proper destruction of the cls/sim classes in
    // the event of an error
    try{
        // Helper class for parsing command line arguments, setting up the
        // simulator, and running the simulator. All of the things done by this
        // class can be done manually if desired. Use the source for the
        // CommandLineSimulator class as a starting point
        sparta::app::CommandLineSimulator cls(USAGE, DEFAULTS);
        // Maintain this code as an example of additional options
        /*
         auto& app_opts = cls.getApplicationOptions();
        app_opts.add_options()
            (VERSION_VARNAME,
             "produce version message",
             "produce version message") // Brief
            ("enable_smart_mcpu",
             sparta::app::named_value<bool>("enable_smart_mcpu", &enable_smart_mcpu)->default_value(false),
             "Enable smart mcpu", "Enable smart mcpu");
        */

        // Add any positional command-line options
        // po::positional_options_description& pos_opts = cls.getPositionalOptions();
        // (void)pos_opts;
        // pos_opts.add(TRACE_POS_VARNAME, -1); // example

        // Parse command line options and configure simulator
        int err_code = EXIT_SUCCESS;
        if(!cls.parse(argc, argv, err_code))
        {
            return err_code; // Any errors already printed to cerr
        }

        // Read general parameters
        const auto upt = cls.getSimulationConfiguration().getUnboundParameterTree();

        // simulation parameters
        auto architecture           = upt.get("meta.params.architecture").getAs<std::string>();
        auto simulation_mode        = upt.get("meta.params.simulation_mode").getAs<std::string>();
        auto trace                  = upt.get("meta.params.trace").getAs<bool>();
        auto events_of_interest     = upt.get("meta.params.events_to_trace").getAs<std::string>();
        auto cmd                    = upt.get("meta.params.cmd").getAs<std::string>();

        cls.getSimulationConfiguration().scheduler_exacting_run=true;

        if(architecture=="tiled")
        {
            setupTiledSimulation(cls);
        }

        // Create the simulator
        sparta::Scheduler scheduler;
        std::shared_ptr<Coyote> sim=std::make_shared<Coyote>(scheduler);

        std::cout << "Simulating: " << cmd;

        cls.populateSimulation(&(*sim));

        // Get a NoC pointer
        coyote::NoC* noc = NULL;
        if(architecture=="tiled")
        {
            noc = sim->getRoot()->getChild(std::string("arch.noc"))->getResourceAs<coyote::NoC>();
        }

        std::shared_ptr<SimulationOrchestrator> orchestrator=NULL;
        
        if(simulation_mode=="execution_driven")
        {
            orchestrator=createExecutionDrivenOrchestrator(cls, sim, noc);
        }
        else if(simulation_mode == "trace_driven")
        {
            orchestrator=createTraceDrivenOrchestrator(cls, sim, noc);
        }

        if(trace)
        {
            coyote::Logger * l=sim->getLogger();

            if(upt.hasValue("meta.params.trace_start_tick") && upt.hasValue("meta.params.trace_end_tick"))
            {
                uint64_t start=upt.get("meta.params.trace_start_tick").getAs<uint64_t>();
                uint64_t end=upt.get("meta.params.trace_end_tick").getAs<uint64_t>();
                std::cout << "Tracing between cycle "<< start << " and cycle " << end << "\n";
                l->setTimeBounds(start, end);
            }
            else
            {
                std::cout << "No bounds specified. Tracing for the whole duration of the application.\n";
            }

            orchestrator->setLogger(l);

            //Remove "[" and "]" from the string
            events_of_interest.erase(0,1);
            events_of_interest.erase(events_of_interest.size()-1);

            std::string delimiter = ",";

            size_t pos = 0;
            std::string token;
            while ((pos = events_of_interest.find(delimiter)) != std::string::npos)
            {
                token = events_of_interest.substr(0, pos);
                if(token!="any")
                {
                    l->addEventOfInterest(token);
                }
                events_of_interest.erase(0, pos + delimiter.length());
            }
            if(events_of_interest!="any")
            {
                l->addEventOfInterest(events_of_interest); //Last event
            }
        }
        orchestrator->run();

        cls.postProcess(&(*sim));

        orchestrator->saveReports();

    } catch(std::stringstream *msg) {
        
        std::cerr << msg->str() << std::endl;
        throw;
    }

    return 0;
}
