// <main.cpp> -*- C++ -*-

#include "SpikeModel.hpp" // Core model skeleton simulator
#include "Request.hpp"
#include "EventManager.hpp"
#include "spike_wrapper.h"
#include "L2SharingPolicy.hpp"
#include "CacheDataMappingPolicy.hpp"
#include "SimulationOrchestrator.hpp"
#include "NoC/DetailedNoC.hpp"

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

//Arbiter arbiter;
int main(int argc, char **argv)
{

    sparta::app::DefaultValues DEFAULTS;
    DEFAULTS.auto_summary_default = "on";
    // try/catch block to ensure proper destruction of the cls/sim classes in
    // the event of an error
    try{
        // Helper class for parsing command line arguments, setting up the
        // simulator, and running the simulator. All of the things done by this
        // classs can be done manually if desired. Use the source for the
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
        auto trace                  = upt.get("meta.params.trace").getAs<bool>();
        auto events_of_interest     = upt.get("meta.params.events_to_trace").getAs<std::string>();
        auto fast_cache             = upt.get("meta.params.fast_cache").getAs<bool>();
        auto cmd                    = upt.get("meta.params.cmd").getAs<std::string>();
        auto enable_smart_mcpu      = upt.get("meta.params.enable_smart_mcpu").getAs<bool>();
        // architectural parameters
        auto isa                    = upt.get("top.cpu.params.isa").getAs<std::string>();
        auto num_tiles              = upt.get("top.cpu.params.num_tiles").getAs<uint16_t>();
        auto num_tiles_per_row      = upt.get("top.cpu.params.num_tiles_per_row").getAs<uint16_t>();
        auto num_cores              = upt.get("top.cpu.params.num_cores").getAs<uint16_t>();
        auto num_threads_per_core   = upt.get("top.cpu.params.num_threads_per_core").getAs<uint16_t>();
        auto thread_switch_latency  = upt.get("top.cpu.params.thread_switch_latency").getAs<uint16_t>();
        auto varch                  = upt.get("top.cpu.params.varch").getAs<std::string>();
        auto icache_config          = upt.get("top.cpu.params.icache_config").getAs<std::string>();
        auto dcache_config          = upt.get("top.cpu.params.dcache_config").getAs<std::string>();
        auto l2bank_line_size       = upt.get("top.cpu.tile0.l2_bank0.params.line_size").getAs<uint64_t>();
        auto l2bank_size            = upt.get("top.cpu.tile0.l2_bank0.params.size_kb").getAs<uint64_t>();
        auto mcpu_line_size         = upt.get("top.cpu.memory_cpu0.params.line_size").getAs<uint64_t>();
        auto num_memory_cpus        = upt.get("top.cpu.params.num_memory_cpus").getAs<uint16_t>();
        auto noc_model              = upt.get("top.cpu.noc.params.noc_model").getAs<std::string>();
        auto bank_policy            = upt.get("top.cpu.tile0.params.bank_policy").getAs<std::string>();
        auto tile_policy            = upt.get("top.cpu.tile0.params.tile_policy").getAs<std::string>();
        auto sharing                = upt.get("top.cpu.tile0.params.l2_sharing_mode").getAs<std::string>();
        auto num_banks              = upt.get("top.cpu.tile0.params.num_l2_banks").getAs<uint16_t>();

        // Copy parameters shared by multiple units
        std::string num_tiles_p ("top.cpu.noc.params.num_tiles");
        cls.getSimulationConfiguration().processParameter(num_tiles_p, sparta::utils::uint32_to_str(num_tiles));
        std::string num_mcpus_p ("top.cpu.noc.params.num_memory_cpus");
        cls.getSimulationConfiguration().processParameter(num_mcpus_p, sparta::utils::uint32_to_str(num_memory_cpus));
        std::string num_mcs_p ("top.cpu.params.num_memory_controllers");
        cls.getSimulationConfiguration().processParameter(num_mcs_p, sparta::utils::uint32_to_str(num_memory_cpus));

        uint8_t bank_and_tile_bits=1;

        if(bank_policy=="set_interleaving")
        {
            bank_and_tile_bits *= num_banks;
        }
        if(tile_policy=="set_interleaving" && sharing=="fully_shared")
        {
            bank_and_tile_bits *= num_tiles;
        }
        cls.getSimulationConfiguration().processParameter("top.cpu.tile*.l2_bank*.params.bank_and_tile_offset", sparta::utils::uint32_to_str(bank_and_tile_bits));

        // Some general parameters checks
        std::string dcache_line_size=dcache_config.substr(dcache_config.rfind(":") + 1);
        std::string icache_line_size=icache_config.substr(icache_config.rfind(":") + 1);
        sparta_assert((l2bank_line_size == mcpu_line_size)            &&
                      (l2bank_line_size == stoul(dcache_line_size)) &&
                      (l2bank_line_size == stoul(icache_line_size)),
            "The L1$D, L1$I, l2_bank and MCPU line size parameters must have the same value. Current values are:" << 
            "\ntop.cpu.tile*.l2_bank*.params.line_size: " << l2bank_line_size <<
            "\ntop.cpu.memory_cpu*.params.line_size: " << mcpu_line_size <<
            "\nL1$D line size: " << dcache_line_size << " because top.cpu.params.dcache_config is: " << dcache_config <<
            "\nL1$I line size: " << icache_line_size << " because top.cpu.params.icache_config is: " << icache_config);

        sparta_assert(num_tiles%num_tiles_per_row == 0, "The number of tiles must be a multiple of the tile per row.");

        // Create the simulator
        sparta::Scheduler scheduler;
        std::shared_ptr<SpikeModel> sim=std::make_shared<SpikeModel>(scheduler);

        std::cout << "Simulating: " << cmd;

        cls.populateSimulation(&(*sim));
        std::shared_ptr<spike_model::EventManager> request_manager=sim->createRequestManager();

        size_t scratchpad_size = ((num_banks * l2bank_size * 1024 * 8) / (num_cores / num_tiles)) / 32;
        std::shared_ptr<spike_model::SpikeWrapper> spike=std::make_shared<spike_model::SpikeWrapper>(
            std::to_string(num_cores),              // Number of cores to simulate
            std::to_string(num_threads_per_core),   // Number of cores in each core
            icache_config,                          // L1$I cache configuration
            dcache_config,                          // L1$D cache configuration
            isa,                                    // RISC-V ISA
            cmd,                                    // Application to execute
            varch,                                  // RISC-V Vector uArch string
            fast_cache,                             // Use a fast L1 cache model instead of the default spike cache
            enable_smart_mcpu,                      // Enable smart mcpu
            scratchpad_size);                       // maximum Virtual vector length

        
        // Get a NoC pointer or NULL to represent non detailed NoC models
        spike_model::DetailedNoC* detailed_noc = NULL;
        if (noc_model == "detailed")
            detailed_noc = sim->getRoot()->getChild(std::string("cpu.noc"))->getResourceAs<spike_model::DetailedNoC>();

        SimulationOrchestrator orchestrator(spike, sim, request_manager, num_cores,
                               num_threads_per_core, thread_switch_latency, trace, detailed_noc);

        if(trace)
        {
            spike_model::Logger l=sim->getLogger();

            if(upt.hasValue("meta.params.trace_start_tick") && upt.hasValue("meta.params.trace_end_tick"))
            {
                uint64_t start=upt.get("meta.params.trace_start_tick").getAs<uint64_t>();
                uint64_t end=upt.get("meta.params.trace_end_tick").getAs<uint64_t>();
                std::cout << "Tracing between cycle "<< start << " and cycle " << end << "\n";
                l.setTimeBounds(start, end);
            }
            else
            {
                std::cout << "No bounds specified. Tracing for the whole duration of the application.\n";
            }

            orchestrator.setLogger(l);

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
                    l.addEventOfInterest(token);
                }
                events_of_interest.erase(0, pos + delimiter.length());
            }
            if(events_of_interest!="any")
            {
                l.addEventOfInterest(events_of_interest); //Last event
            }
        }
        orchestrator.run();

        cls.postProcess(&(*sim));

    }catch(...){
        // Could still handle or log the exception here
        throw;
    }

    return 0;
}
