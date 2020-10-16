// <main.cpp> -*- C++ -*-

#include "SpikeModel.hpp" // Core model skeleton simulator
#include "L2Request.hpp"
#include "RequestManager.hpp"
#include "spike_wrapper.h"
#include "L2SharingPolicy.hpp"
#include "DataMappingPolicy.hpp"
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
    uint32_t num_tiles = 1;
    uint32_t num_l2_banks = 1;
    uint32_t num_memory_controllers = 1;
    std::string application="";
    std::string isa="RV64";
    std::string p="1";
    std::string ic;
    std::string dc;
    std::string cmd;
    std::string varch;
    std::string l2_sharing;
    std::string bank_policy;
    std::string tile_policy;
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
             sparta::app::named_value<std::string>("isa", &isa)->default_value("RV64"),
             "The RISC-V isa version to use", "The RISC-V isa version to use")
            ("num_tiles",
             sparta::app::named_value<std::uint32_t>("num_tiles", &num_tiles)->default_value(1),
             "The number of tiles to simulate", "The number of tiles to simulate (must be a divider of the number of cores)")
            ("p",
             sparta::app::named_value<std::string>("NUM_CORES", &p)->default_value("1"),
             "The total number of cores to simulate", "The number of cores to simulate")
            ("num_l2_banks",
             sparta::app::named_value<std::uint32_t>("NUM_BANKS", &num_l2_banks)->default_value(1),
             "The number of L2 banks per tile", "The number of L2 banks per tile")
            ("num_memory_controllers",
             sparta::app::named_value<std::uint32_t>("NUM_MCS", &num_memory_controllers)->default_value(1),
             "The number of memory controllers", "The number of memory controllers")
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
            ("l2_sharing_mode",
             sparta::app::named_value<std::string>("POLICY", &l2_sharing)->default_value("tile_private"),
             "The L2 sharing mode", "Supported values: tile_private, fully_shared")
            ("bank_data_mapping_policy",
             sparta::app::named_value<std::string>("POLICY", &bank_policy)->default_value("page_to_bank"),
             "The data mapping policy for the accesses to banks within a tile.", "Supported values: page_to_bank, set_interleaving")
            ("tile_data_mapping_policy",
             sparta::app::named_value<std::string>("POLICY", &tile_policy)->default_value("tile_private"),
             "The data mapping policy for the accesses to remote tiles.", "Ignored if l2_sharing_mode=tile_private, supported values: page_to_bank, set_interleaving");

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
        
        num_cores=stoi(p);
     
        std::string noc_tiles("top.cpu.noc.params.num_tiles");
        std::string noc_mcs("top.cpu.noc.params.num_memory_controllers");
//        std::string spike_cores("top.cpu.spike.params.p");
       
        //Here I set several model paramteres using a single arg to make usage easier 
        cls.getSimulationConfiguration().processParameter(noc_tiles, sparta::utils::uint32_to_str(num_tiles));
        cls.getSimulationConfiguration().processParameter(noc_mcs, sparta::utils::uint32_to_str(num_memory_controllers));
//        cls.getSimulationConfiguration().processParameter(spike_cores, sparta::utils::uint32_to_str(num_cores));


        for(uint32_t i=0;i<num_tiles;i++)
        {
            std::string tile_bank("top.cpu.tile*.params.num_l2_banks");
            size_t start_pos = tile_bank.find("*");
            tile_bank.replace(start_pos, 1, std::to_string(i));
            cls.getSimulationConfiguration().processParameter(tile_bank, sparta::utils::uint32_to_str(num_l2_banks));
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

        spike_model::DataMappingPolicy b_pol=spike_model::DataMappingPolicy::PAGE_TO_BANK;
        if(bank_policy=="page_to_bank")
        {
            b_pol=spike_model::DataMappingPolicy::PAGE_TO_BANK;
        }
        else if(bank_policy=="set_interleaving")
        {
            b_pol=spike_model::DataMappingPolicy::SET_INTERLEAVING;
        }

        spike_model::DataMappingPolicy t_pol=spike_model::DataMappingPolicy::PAGE_TO_BANK;
        if(tile_policy=="page_to_bank")
        {
            t_pol=spike_model::DataMappingPolicy::PAGE_TO_BANK;
        }
        else if(tile_policy=="set_interleaving")
        {
            t_pol=spike_model::DataMappingPolicy::SET_INTERLEAVING;
        }

        // Create the simulator
        sparta::Scheduler scheduler;
        std::shared_ptr<SpikeModel> sim=std::make_shared<SpikeModel>("core_topology_4",
                             scheduler,
                             num_cores/num_tiles,
                             num_tiles,
                             num_l2_banks,
                             num_memory_controllers,
                             l2_sharing_policy,
                             b_pol,
                             t_pol,
                             application, //application to simulate
                             isa,
                             show_factories,
                             trace); // run for ilimit instructions

        std::cout << "Simulating: " << application;

        cls.populateSimulation(&(*sim));
        
        std::shared_ptr<spike_model::RequestManager> request_manager=sim->createRequestManager();

        std::shared_ptr<spike_model::SpikeWrapper> spike=std::make_shared<spike_model::SpikeWrapper>(p,ic,dc,isa,cmd,varch);

        //printf("Creqated\n");

        SimulationOrchestrator orchestrator(spike, sim, request_manager, num_cores);

        orchestrator.run();

        /*std::vector<uint16_t> active_cores;
        std::vector<uint16_t> stalled_cores;
        std::vector<std::list<std::shared_ptr<spike_model::L2Request>>> pending_misses_per_core(num_cores);
        std::vector<std::list<std::shared_ptr<spike_model::L2Request>>> pending_writebacks_per_core(num_cores);
        std::vector<uint64_t> simulated_instructions_per_core(num_cores);

        for(uint16_t i=0;i<num_cores;i++)
        {
            active_cores.push_back(i);
        }


        uint64_t current_cycle=0;
        uint64_t next_event_tick=sparta::Scheduler::INDEFINITE;

        uint64_t d=0;        

        bool spike_finished=false;
        
        //While there are events or instructions to simulate
        while(!scheduler.isFinished() || !spike_finished)
        {
            //printf("Current %lu, next %lu\n", current_cycle, next_event_tick);
            
            //EXECUTE INSTS IN THE ACTIVE CORES
            for(uint16_t i=0;i<active_cores.size();i++)
            {
                uint16_t core=active_cores[i];

                simulated_instructions_per_core[core]++;

                std::list<std::shared_ptr<spike_model::L2Request>> new_misses;
                //auto t1 = std::chrono::high_resolution_clock::now();
                bool success=spike.simulateOne(core, current_cycle, new_misses);
                //auto t2 = std::chrono::high_resolution_clock::now();
                //d += std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();

                bool core_finished=false;
                bool running=true;

                if(new_misses.size()>0)
                {
                    //FETCH MISSES ARE SERVICED WHETHER THERE IS A RAW OR NOT
                    if(new_misses.front()->getType()==spike_model::L2Request::AccessType::FETCH)
                    {
                        uint64_t lapse=0;
                        if(current_cycle-scheduler.getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                        {
                            lapse=current_cycle-scheduler.getCurrentTick();
                        }
                        request_manager->putRequest(new_misses.front(), lapse);
                        new_misses.pop_front();
                        running=false;
                    }
                    else if(new_misses.front()->getType()==spike_model::L2Request::AccessType::FINISH)
                    {
                        running=false;
                        core_finished=true;
                    }
                }

                //IF NO RAW AND NO FETCH MISS
                if(success && running)
                {
                    //count_simulated_instructions_++;
                    if(new_misses.size()>0)
                    {
                        for(std::shared_ptr<spike_model::L2Request> miss: new_misses)
                        {
                            if(miss->getType()!=spike_model::L2Request::AccessType::WRITEBACK)
                            {
                                uint64_t lapse=0;
                                if(current_cycle-scheduler.getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                                {
                                    lapse=current_cycle-scheduler.getCurrentTick();
                                }
                                request_manager->putRequest(miss, lapse);
                            }
                            else //WRITEBACKS ARE HANDLED LAST
                            {
                                pending_writebacks_per_core[i].push_back(miss);
                            }
                        }
                    }

                }
                else
                {
                    if(trace_)
                    {
                        logger_.logStall(getClock()->currentCycle(), id_);
                    }
                    
                    if(running)
                    {
                        //count_dependency_stalls_++;
                        running=false;
                    }
                    else
                    {
                        //fetch_stalls_++;
                    }
                    for(std::shared_ptr<spike_model::L2Request> miss: new_misses)
                    {   
                        pending_misses_per_core[i].push_back(miss); //Instructions are not replayed, so we have to store the misses of a raw or under a fetch, so they are serviced later
                    }
                }

                if(!running)
                {
                    active_cores.erase(active_cores.begin()+i);
                    i--; //We have deleted an element, so we have to update the index that we are using to traverse the data structure
                    if(!core_finished)
                    {
                        //printf("Stalling core %d. Size: %lu\n", core, stalled_cores.size());
                        stalled_cores.push_back(core);
                    }
                    else
                    {
                        //printf("HERE %lu %lu\n", active_cores.size(), stalled_cores.size());
                        //Core is not added back to any structure
                        //Spike has finished if all the cores have finished
                        if(active_cores.size()==0 && stalled_cores.size()==0)
                        {
                            spike_finished=true;
                        }
                    }
                }
            }

            //GET NEXT EVENT
            if(next_event_tick==sparta::Scheduler::INDEFINITE)
            {
                next_event_tick=scheduler.nextEventTick();
                //printf("NEXT updated to %lu\n", next_event_tick);
            }
           
            //HANDLE ALL THE EVENTS FOR THE CURRENT CYCLE
            if(next_event_tick==current_cycle)
            {
                uint64_t advance=next_event_tick-scheduler.getCurrentTick()+1; //Add 1 because the  run method is not inclusive
                scheduler.run(advance, true);
                //Now the scheduler has fully executed as many ticks as spike (scheduler.getCurrentTick()==current_cycle+1 && scheduler.getElapsedTicks()==current_cycle)
                next_event_tick=scheduler.nextEventTick();
                //Check serviced requests
                while(request_manager->hasServicedRequest())
                {
                    std::shared_ptr<spike_model::L2Request> req=request_manager->getServicedRequest();
                    uint16_t core=req->getCoreId();
                    if(pending_misses_per_core[core].size()>0) //If this was a fetch or there was a raw and there are data misses for the instructions
                    {
                        while(pending_misses_per_core[core].size()>0)
                        {
                            std::shared_ptr<spike_model::L2Request> miss=pending_misses_per_core[core].front();
                            pending_misses_per_core[core].pop_front();
                            uint64_t lapse=0;
                            if(current_cycle-scheduler.getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                            {
                                lapse=current_cycle-scheduler.getCurrentTick();
                            }
                            request_manager->putRequest(miss, lapse);
                        }
                    }
                    else
                    {
                        while(pending_writebacks_per_core[core].size()>0) //Writebacks are sent last
                        {
                            std::shared_ptr<spike_model::L2Request> miss=pending_writebacks_per_core[core].front();
                            pending_writebacks_per_core[core].pop_front();
                            uint64_t lapse=0;
                            if(current_cycle-scheduler.getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                            {
                                lapse=current_cycle-scheduler.getCurrentTick();
                            }
                            request_manager->putRequest(miss, lapse);
                        }
                    }

                    bool can_run=true;
                    bool is_load=req->getType()==spike_model::L2Request::AccessType::LOAD;

                    if(is_load)
                    {
                        //printf("Sending the corresponding ack to register %d on tick %lu\n", req->getRegId(), current_cycle);
                        //What about vectors??? More than one miss might be necessary to free a single register
                        can_run=spike.ackRegister(req, current_cycle);
                    }
                    if(can_run)
                    {
                        //printf("CAN RUN is true\n");
                        std::vector<uint16_t>::iterator it;

                        it=std::find(stalled_cores.begin(), stalled_cores.end(), core);
                        if (it != stalled_cores.end())
                        {
                            //printf("Notifying dependence on %d\n", req->getRegId());
                            stalled_cores.erase(it);
                            active_cores.push_back(core); 
                        }
                        if(trace_)
                        {
                            logger_.logResume(getClock()->currentCycle(), id_);
                        }
                    }
                }
            }
            
            //ADVANCE CLOCK
            if(active_cores.size()==0 && next_event_tick!=sparta::Scheduler::INDEFINITE)
            {
                current_cycle=next_event_tick;
            }
            else
            {
                current_cycle++;
            }
        }
        
        //printf("After LOOP\n");

        current_cycle--;
        
        */

        //sim.run(1);    
        
        cls.postProcess(&(*sim));
        /*uint64_t tot=0;
        for(uint16_t i=0;i<num_cores;i++)
        {
            printf("Core %d simulated %lu instructions\n", i, simulated_instructions_per_core[i]);
            tot+=simulated_instructions_per_core[i];
        }
        printf("Total simulated instructions %lu\n", tot);
        printf("The monitored section took %lu nanoseconds\n", d);
        */

    }catch(...){
        // Could still handle or log the exception here
        throw;
    }

    return 0;
}
