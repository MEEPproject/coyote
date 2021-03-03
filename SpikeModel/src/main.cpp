// <main.cpp> -*- C++ -*-

#include "SpikeModel.hpp" // Core model skeleton simulator
#include "Request.hpp"
#include "RequestManagerIF.hpp"
#include "spike_wrapper.h"
#include "L2SharingPolicy.hpp"
#include "CacheDataMappingPolicy.hpp"
#include "SimulationOrchestrator.hpp"

#include "sparta/parsers/ConfigEmitterYAML.hpp"
#include "sparta/app/CommandLineSimulator.hpp"
#include "sparta/app/MultiDetailOptions.hpp"
#include "sparta/sparta.hpp"

#include <iostream>
#include <string>

#include <chrono>

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


int main(int argc, char **argv)
{
    std::vector<std::string> datafiles;
    uint32_t num_cores = 1;
    uint32_t num_threads_per_core = 1;
    uint64_t thread_switch_latency = 0;
    uint32_t num_tiles = 1;
    uint32_t num_l2_banks = 1;
    uint32_t num_memory_cpus = 1;
    uint32_t num_memory_controllers = 1;
    uint32_t num_memory_banks = 8;
    std::string application="";
    std::string isa="RV64";
    std::string p="1";
    std::string t="1";
    std::string l="0";
    std::string ic;
    std::string dc;
    std::string cmd;
    std::string varch;
    std::string address_mapping;
    std::string l2_sharing;
    std::string bank_policy;
    std::string tile_policy;
    bool fast_cache;
    bool trace=false;

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
        auto& app_opts = cls.getApplicationOptions();
        app_opts.add_options()
            /*(VERSION_VARNAME,
             "produce version message",
             "produce version message") // Brief*/
            ("show-factories",
             "Show the registered factories")
            ("trace",
             sparta::app::named_value<bool>("trace", &trace)->default_value(false),
             "Whether tracing is enabled or not", "Whether tracing is enabled or not")
            ("isa",
             sparta::app::named_value<std::string>("isa", &isa)->default_value("RV64IMAFDCV"),
             "The RISC-V isa version to use", "The RISC-V isa version to use")
            ("num_tiles",
             sparta::app::named_value<std::uint32_t>("num_tiles", &num_tiles)->default_value(1),
             "The number of tiles to simulate", "The number of tiles to simulate (must be a divider of the number of cores)")
            ("p",
             sparta::app::named_value<std::string>("NUM_CORES", &p)->default_value("1"),
             "The total number of cores to simulate", "The number of cores to simulate")
            ("t",
             sparta::app::named_value<std::string>("NUM_THREADS_PER_CORE", &t)->default_value("1"),
             "The total number of threads per core to simulate", "The number of threads per cores to simulate")
            ("l",
             sparta::app::named_value<std::string>("THREAD_SWITCH_LATENCY", &l)->default_value("0"),
             "Number of cycles required to make the thread runnable", "Number of cycles required to make the thread runnable")
            ("num_l2_banks",
             sparta::app::named_value<std::uint32_t>("NUM_CACHE_BANKS", &num_l2_banks)->default_value(1),
             "The number of L2 banks per tile", "The number of L2 banks per tile")
            ("num_memory_cpus",
             sparta::app::named_value<std::uint32_t>("NUM_MCPUS", &num_memory_cpus)->default_value(1),
             "the number of memory cpus", "the number of memory cpus")
            ("num_memory_controllers",
             sparta::app::named_value<std::uint32_t>("NUM_MCS", &num_memory_controllers)->default_value(1),
             "the number of memory controllers", "the number of memory controllers")
            ("num_memory_banks",
             sparta::app::named_value<std::uint32_t>("NUM_MEMORY_BANKS", &num_memory_banks)->default_value(8),
             "The number of memory banks handled by each memory controller", "The number of memory banks")
            ("ic",
             sparta::app::named_value<std::string>("ic", &ic)->default_value("64:8:64"),
             "The icache configuration", "The icache configuration")
            ("dc",
             sparta::app::named_value<std::string>("dc", &dc)->default_value("64:8:64"),
             "The dcache configuration", "The dcache configuration")
            ("cmd",
             sparta::app::named_value<std::string>("cmd", &cmd)->default_value(""),
             "The command to simulate", "The command to simulate")
            ("varch",
             sparta::app::named_value<std::string>("varch", &varch)->default_value("v128:e64:s128"),
             "The varch to use", "The varch to use")
            ("address_mapping_mode",
             sparta::app::named_value<std::string>("POLICY", &address_mapping)->default_value("open_page"),
             "The memory addressing mode", "Supported values: open_page, close_page")
            ("l2_sharing_mode",
             sparta::app::named_value<std::string>("POLICY", &l2_sharing)->default_value("tile_private"),
             "The L2 sharing mode", "Supported values: tile_private, fully_shared")
            ("bank_data_mapping_policy",
             sparta::app::named_value<std::string>("POLICY", &bank_policy)->default_value("page_to_bank"),
             "The data mapping policy for the accesses to banks within a tile.", "Supported values: page_to_bank, set_interleaving")
            ("tile_data_mapping_policy",
             sparta::app::named_value<std::string>("POLICY", &tile_policy)->default_value("tile_private"),
             "The data mapping policy for the accesses to remote tiles.", "Ignored if l2_sharing_mode=tile_private, supported values: page_to_bank, set_interleaving")
            ("fast_cache",
             sparta::app::named_value<bool>("TRUE/FALSE", &fast_cache)->default_value(false),
             "Whether to use a fast L1 cache model instead of the default spike cache.", "Whether to use a fast L1 cache model instead of the default spike cache.");

        // Add any positional command-line options
        // po::positional_options_description& pos_opts = cls.getPositionalOptions();
        // (void)pos_opts;
        // pos_opts.add(TRACE_POS_VARNAME, -1); // example


        // Parse command line options and configure simulator
        int err_code = 0;
        if(!cls.parse(argc, argv, err_code)){
            return err_code; // Any errors already printed to cerr
        }

        bool show_factories = false;
        auto& vm = cls.getVariablesMap();
        if(vm.count("show-factories") != 0) {
            show_factories = true;
        }

        num_threads_per_core = stoi(t);
        thread_switch_latency = stoi(l);
        num_cores=stoi(p);

        std::string noc_tiles("top.cpu.noc.params.num_tiles");
        std::string noc_mcs("top.cpu.noc.params.num_memory_cpus");
//        std::string spike_cores("top.cpu.spike.params.p");

        //Here I set several model paramteres using a single arg to make usage easier
        cls.getSimulationConfiguration().processParameter(noc_tiles, sparta::utils::uint32_to_str(num_tiles));
        cls.getSimulationConfiguration().processParameter(noc_mcs, sparta::utils::uint32_to_str(num_memory_cpus));
//        cls.getSimulationConfiguration().processParameter(spike_cores, sparta::utils::uint32_to_str(num_cores));


        std::string line_size=dc.substr(dc.rfind(":") + 1);

        //Set the number of banks in each tile and the line size.
        for(uint32_t i=0;i<num_tiles;i++)
        {
            std::string tile_bank("top.cpu.tile*.params.num_l2_banks");
            size_t start_pos = tile_bank.find("*");
            tile_bank.replace(start_pos, 1, std::to_string(i));
            cls.getSimulationConfiguration().processParameter(tile_bank, sparta::utils::uint32_to_str(num_l2_banks));
        }

        //Set the line size in each bank
        for(uint32_t i=0;i<num_tiles;i++)
        {
            std::string tile("top.cpu.tile*.l2_bank&.params.line_size");
            size_t start_pos = tile.find("*");
            tile.replace(start_pos, 1, std::to_string(i));
            for(uint32_t j=0;j<num_l2_banks;j++)
            {
                std::string bank=tile;
                size_t start_pos = bank.find("&");
                std::cout << bank;
                bank.replace(start_pos, 1, std::to_string(j));
                cls.getSimulationConfiguration().processParameter(bank, line_size);
            }
        }

        for(std::size_t i = 0; i < num_memory_controllers; ++i)
        {
            std::string mc_line("top.cpu.memory_controller*.params.line_size");
            size_t start_pos = mc_line.find("*");
            mc_line.replace(start_pos, 1, std::to_string(i));
            cls.getSimulationConfiguration().processParameter(mc_line, line_size);

            std::string mc_bank("top.cpu.memory_controller*.params.num_banks");
            start_pos = mc_bank.find("*");
            mc_bank.replace(start_pos, 1, std::to_string(i));
            cls.getSimulationConfiguration().processParameter(mc_bank, sparta::utils::uint32_to_str(num_memory_banks));
        }

        spike_model::L2SharingPolicy l2_sharing_policy=spike_model::L2SharingPolicy::TILE_PRIVATE;
        if(l2_sharing=="tile_private")
        {
            l2_sharing_policy=spike_model::L2SharingPolicy::TILE_PRIVATE;
        }
        else if(l2_sharing=="fully_shared")
        {
            l2_sharing_policy=spike_model::L2SharingPolicy::FULLY_SHARED;
        }

        spike_model::CacheDataMappingPolicy b_pol=spike_model::CacheDataMappingPolicy::PAGE_TO_BANK;
        if(bank_policy=="page_to_bank")
        {
            b_pol=spike_model::CacheDataMappingPolicy::PAGE_TO_BANK;
        }
        else if(bank_policy=="set_interleaving")
        {
            b_pol=spike_model::CacheDataMappingPolicy::SET_INTERLEAVING;
        }

        spike_model::CacheDataMappingPolicy t_pol=spike_model::CacheDataMappingPolicy::PAGE_TO_BANK;
        if(tile_policy=="page_to_bank")
        {
            t_pol=spike_model::CacheDataMappingPolicy::PAGE_TO_BANK;
        }
        else if(tile_policy=="set_interleaving")
        {
            t_pol=spike_model::CacheDataMappingPolicy::SET_INTERLEAVING;
        }

        spike_model::AddressMappingPolicy a_pol=spike_model::AddressMappingPolicy::OPEN_PAGE;
        if(address_mapping=="open_page")
        {
            a_pol=spike_model::AddressMappingPolicy::OPEN_PAGE;
        }
        else if(address_mapping=="close_page")
        {
            a_pol=spike_model::AddressMappingPolicy::CLOSE_PAGE;
        }

        // Create the simulator
        sparta::Scheduler scheduler;
        std::shared_ptr<SpikeModel> sim=std::make_shared<SpikeModel>("core_topology_4",
                             scheduler,
                             num_cores/num_tiles,
                             num_tiles,
                             num_l2_banks,
                             num_memory_cpus,
                             num_memory_controllers,
                             num_memory_banks,
                             a_pol,
                             l2_sharing_policy,
                             b_pol,
                             t_pol,
                             application, //application to simulate
                             isa,
                             show_factories,
                             trace); // run for ilimit instructions

        std::cout << "Simulating: " << application;

        cls.populateSimulation(&(*sim));

        std::shared_ptr<spike_model::RequestManagerIF> request_manager=sim->createRequestManager();


        std::shared_ptr<spike_model::SpikeWrapper> spike=std::make_shared<spike_model::SpikeWrapper>(p,t,ic,dc,isa,cmd,varch,fast_cache);

        //printf("Creqated\n");

        SimulationOrchestrator orchestrator(spike, sim, request_manager, num_cores,
                               num_threads_per_core, thread_switch_latency, trace);

        if(trace)
        {
            orchestrator.setLogger(sim->getLogger());
        }
        orchestrator.run();

        cls.postProcess(&(*sim));

    }catch(...){
        // Could still handle or log the exception here
        throw;
    }

    return 0;
}
