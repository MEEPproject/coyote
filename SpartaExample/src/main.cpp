// <main.cpp> -*- C++ -*-

#include "SpartaExample.hpp"

#include "sparta/parsers/ConfigEmitterYAML.hpp"
#include "sparta/app/CommandLineSimulator.hpp"
#include "sparta/app/MultiDetailOptions.hpp"
#include "sparta/sparta.hpp"
#include "sparta/utils/SpartaAssert.hpp"

#include <iostream>
#include <string> 
#include <chrono>
#include "stdlib.h"

const char USAGE[] =
    "Usage:\n"
    "    [--show-tree] [--show-dag]\n"
    "    [-p PATTERN VAL] [-c FILENAME]\n"
    "    [--node-config-file PATTERN FILENAME]\n"
    "    [-l PATTERN CATEGORY DEST]\n"
    "    [-h]\n"
    "\n";

constexpr char VERSION_VARNAME[] = "version";
constexpr char DATA_FILE_VARNAME[] = "data-file";
constexpr char DATA_FILE_OPTIONS[] = "data-file";
                
int main(int argc, char **argv)
{

    sparta::app::DefaultValues DEFAULTS;
    DEFAULTS.auto_summary_default = "on";

    try{
        sparta::app::CommandLineSimulator cls(USAGE, DEFAULTS);
        
        int err_code = EXIT_SUCCESS;
        if(!cls.parse(argc, argv, err_code))
        {
            return err_code;
        }


        cls.getSimulationConfiguration().scheduler_exacting_run=true;

        sparta::Scheduler scheduler;

        //Creates the simulator. This is equivalent to the creation of a Coyote class instance in the Coyote sim
        minimum_two_phase_example::SpartaExample sim(scheduler);

        //Build all the Sparta units in the simulator and bind them
        cls.populateSimulation(&sim);

        //Run the simulator to completion. Actual Coyote would use the orchestrator to do the right thing in each cycle
        cls.runSimulator(&sim);

        cls.postProcess(&sim);

    }catch(...){
        throw;
    }

    return 0;
}
