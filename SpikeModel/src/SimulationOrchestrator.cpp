#include "SimulationOrchestrator.hpp"

SimulationOrchestrator::SimulationOrchestrator(std::shared_ptr<spike_model::SpikeWrapper>& spike, std::shared_ptr<SpikeModel>& spike_model, std::shared_ptr<spike_model::RequestManagerIF>& request_manager, uint32_t num_cores, bool trace):
    spike(spike),
    spike_model(spike_model),
    request_manager(request_manager),
    num_cores(num_cores),
    pending_misses_per_core(num_cores),
    pending_writebacks_per_core(num_cores),
    simulated_instructions_per_core(num_cores),
    current_cycle(1),
    next_event_tick(sparta::Scheduler::INDEFINITE),
    timer(0),
    spike_finished(false),
    trace(trace)
{
    for(uint16_t i=0;i<num_cores;i++)
    {
        active_cores.push_back(i);
    }
    
}
        
uint64_t SimulationOrchestrator::spartaDelay(uint64_t cycle)
{
    return cycle-spike_model->getScheduler()->getCurrentTick();
}

void SimulationOrchestrator::simulateInstInActiveCores()
{
    for(uint16_t i=0;i<active_cores.size();i++)
    {
        uint16_t core=active_cores[i];

        simulated_instructions_per_core[core]++;

        if(trace && simulated_instructions_per_core[core]%1000==0)
        {
           logger_.logKI(current_cycle, core); 
        }

        std::list<std::shared_ptr<spike_model::Request>> new_misses;
        //auto t1 = std::chrono::high_resolution_clock::now();
        bool success=spike->simulateOne(core, current_cycle, new_misses);
        //auto t2 = std::chrono::high_resolution_clock::now();
        //timer += std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();


        bool core_finished=false;
        bool active=true;

        if(new_misses.size()>0)
        {
            if(new_misses.front()->getType()==spike_model::Request::AccessType::FETCH)
            {
                //Fetch misses are serviced whether there is a raw or not. Subsequent, misses will be submitted later.
                uint64_t lapse=1;
                if(current_cycle-spike_model->getScheduler()->getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                {
                    lapse=lapse+spartaDelay(current_cycle);
                }
                request_manager->putRequest(new_misses.front(), lapse);
                new_misses.pop_front();
                active=false;
            }
            else if(new_misses.front()->getType()==spike_model::Request::AccessType::FINISH)
            {
                active=false;
                core_finished=true;
            }
        }

        //IF NO RAW AND NO FETCH MISS
        if(success && active)
        {
            //Manage each of the pending misses
            for(std::shared_ptr<spike_model::Request> miss: new_misses)
            {
                if(miss->getType()!=spike_model::Request::AccessType::WRITEBACK)
                {
                    uint64_t lapse=1;
                    if(current_cycle-spike_model->getScheduler()->getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                    {
                        lapse=lapse+spartaDelay(current_cycle);
                    }
                    request_manager->putRequest(miss, lapse);
                }
                else //WRITEBACKS ARE HANDLED LAST
                {
                    pending_writebacks_per_core[core].push_back(miss);
                }
            }
        }
        else
        {
            active=false;
            for(std::shared_ptr<spike_model::Request> miss: new_misses)
            {  
                pending_misses_per_core[core].push_back(miss); //Instructions are not replayed, so we have to store the misses of a raw or under a fetch, so they are serviced later
            }
            
            if(trace_)
            {
                logger_.logStall(current_cycle, core, 0);
            }
        }

        if(!active)
        {
            active_cores.erase(active_cores.begin()+i);

            i--; //We have deleted an element, so we have to update the index that we are using to traverse the data structure

            if(!core_finished)
            {
                //The core is not active and is not finished, so it goe into the stalled cores list
                stalled_cores.push_back(core);
            }
            else
            {
                //Core is not added back to any structure
                //Spike has finished if all the cores have finished
                if(active_cores.size()==0 && stalled_cores.size()==0)
                {
                    spike_finished=true;
                }
            }
        }
    }
}

void SimulationOrchestrator::handleEvents()
{
    //GET NEXT EVENT
    if(next_event_tick==sparta::Scheduler::INDEFINITE)
    {
        next_event_tick=spike_model->getScheduler()->nextEventTick();
    }
   
    //HANDLE ALL THE EVENTS FOR THE CURRENT CYCLE
    if(next_event_tick==current_cycle)
    {
        //Obtains how much sparta needs to advance. Add 1 because the  run method is not inclusive.
        uint64_t advance=spartaDelay(next_event_tick)+1;
        spike_model->runRaw(advance);
        //Now the Sparta Scheduler and the Orchestrator are in sync

        next_event_tick=spike_model->getScheduler()->nextEventTick();

        //Check serviced requests
        while(request_manager->hasServicedRequest())
        {
            std::shared_ptr<spike_model::Request> req=request_manager->getServicedRequest();
            uint16_t core=req->getCoreId();

            if(pending_misses_per_core[core].size()>0) //If this core has pending misses (previous fetch or raw), handle them
            {
                while(pending_misses_per_core[core].size()>0)
                {
                    std::shared_ptr<spike_model::Request> miss=pending_misses_per_core[core].front();
                    pending_misses_per_core[core].pop_front();
                    uint64_t lapse=1;
                    if(current_cycle-spike_model->getScheduler()->getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                    {
                        lapse=lapse+current_cycle-spartaDelay(current_cycle);
                    }
                    request_manager->putRequest(miss, lapse);
                }
            }
            else
            {
                while(pending_writebacks_per_core[core].size()>0) //Writebacks are handled last
                {
                    std::shared_ptr<spike_model::Request> miss=pending_writebacks_per_core[core].front();
                    pending_writebacks_per_core[core].pop_front();
                    uint64_t lapse=0;
                    if(current_cycle-spike_model->getScheduler()->getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                    {
                        lapse=lapse+spartaDelay(current_cycle);
                    }
                    request_manager->putRequest(miss, lapse);
                }
            }

            bool can_run=true;
            bool is_load=req->getType()==spike_model::Request::AccessType::LOAD;

            //Ack the registers if this is a load
            if(is_load)
            {
                can_run=spike->ackRegister(req, current_cycle);
            }

            if(can_run)
            {
                std::vector<uint16_t>::iterator it;

                it=std::find(stalled_cores.begin(), stalled_cores.end(), core);

                //If the core was stalled, make it active again
                if (it != stalled_cores.end())
                {
                    stalled_cores.erase(it);
                    active_cores.push_back(core);
                    if(trace_)
                    {
                        logger_.logResume(current_cycle, core, 0);
                    }
                }
            }
        }
    }
}

void SimulationOrchestrator::run()
{    
    //Each iteration of the loop handles a cycle
    //Simulation will end when there are neither pending events nor more instructions to simulate
    while(!spike_model->getScheduler()->isFinished() || !spike_finished)
    {
//        printf("Current %lu, next %lu. Bools: %lu, %lu. Insts: %lu\n", current_cycle, next_event_tick, active_cores.size(), stalled_cores.size(), simulated_instructions_per_core[0]);
        
        simulateInstInActiveCores();
        handleEvents();
    
        //If there are no active cores and there is a pending event
        if(active_cores.size()==0 && next_event_tick!=sparta::Scheduler::INDEFINITE)
        {
            //Advance the clock to the cycle for the event
            current_cycle=next_event_tick;
        }
        else
        {
            //Advance clock to next
            current_cycle++;
        }
    }
    
    //PRINTS THE SPARTA STATISTICS
    spike_model->saveReports();

    uint64_t tot=0;
    for(uint16_t i=0;i<num_cores;i++)
    {
        printf("Core %d simulated %lu instructions\n", i, simulated_instructions_per_core[i]);
        tot+=simulated_instructions_per_core[i];
    }
    printf("Total simulated instructions %lu\n", tot);
    printf("The monitored section took %lu nanoseconds\n", timer);
}
