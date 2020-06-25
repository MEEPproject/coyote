// <main.cpp> -*- C++ -*-

#include "SpikeModel.hpp" // Core model skeleton simulator


#include "sparta/parsers/ConfigEmitterYAML.hpp"
#include "sparta/app/CommandLineSimulator.hpp"
#include "sparta/app/MultiDetailOptions.hpp"
#include "sparta/sparta.hpp"

#include <iostream>


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
    std::string application="";
    std::string isa="RV64";

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
            (VERSION_VARNAME,
             "produce version message",
             "produce version message") // Brief
            ("num-cores",
             sparta::app::named_value<uint32_t>("CORES", &num_cores)->default_value(1),
             "The number of cores in simulation", "The number of cores in simulation")
            ("show-factories",
             "Show the registered factories")
            ("application",
             sparta::app::named_value<std::string>("application", &application)->default_value(""),
             "The application to simulate (including its args)", "The application to simulate (including its args)")
            ("isa",
             sparta::app::named_value<std::string>("isa", &isa)->default_value("RV64"),
             "The RISC-V isa version to use", "The RISC-V isa version to use");

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
        
        std::string noc_cores("top.cpu.noc.params.num_cores");
        std::string spike_cores("top.cpu.spike.params.p");
       
        //Here I set several model paramteres using a single arg to make usage easier 
        cls.getSimulationConfiguration().processParameter(noc_cores, sparta::utils::uint32_to_str(num_cores));
        cls.getSimulationConfiguration().processParameter(spike_cores, sparta::utils::uint32_to_str(num_cores));

        std::string noc_banks("top.cpu.noc.params.num_l2_banks");
        uint32_t num_l2_banks=(uint32_t) std::stoi(cls.getSimulationConfiguration().getUnboundParameterTree().get(noc_banks).getValue());
        printf("There are %d banks\n", num_l2_banks);

        // Create the simulator
        sparta::Scheduler scheduler;
        SpikeModel sim("core_topology_4",
                             scheduler,
                             num_cores, // cores
                             num_l2_banks,
                             application, //application to simulate
                             isa,
                             show_factories); // run for ilimit instructions

        std::cout << "Simulating: " << application;

        cls.populateSimulation(&sim);

        cls.runSimulator(&sim);

        cls.postProcess(&sim);

    }catch(...){
        // Could still handle or log the exception here
        throw;
    }

    return 0;
}
