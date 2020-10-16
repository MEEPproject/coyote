#include "SimulationOrchestrator.hpp"

SimulationOrchestrator::SimulationOrchestrator(std::shared_ptr<spike_model::SpikeWrapper>& spike, std::shared_ptr<SpikeModel>& spike_model, std::shared_ptr<spike_model::RequestManager>& request_manager, uint32_t num_cores):
    spike(spike),
    spike_model(spike_model),
    request_manager(request_manager),
    num_cores(num_cores),
    pending_misses_per_core(num_cores),
    pending_writebacks_per_core(num_cores),
    simulated_instructions_per_core(num_cores),
    current_cycle(0),
    next_event_tick(sparta::Scheduler::INDEFINITE),
    timer(0),
    spike_finished(false)
{
    for(uint16_t i=0;i<num_cores;i++)
    {
        active_cores.push_back(i);
    }
    
}

void SimulationOrchestrator::simulateInstInActiveCores()
{
    for(uint16_t i=0;i<active_cores.size();i++)
    {
        uint16_t core=active_cores[i];

        simulated_instructions_per_core[core]++;

        std::list<std::shared_ptr<spike_model::L2Request>> new_misses;
        //auto t1 = std::chrono::high_resolution_clock::now();
        bool success=spike->simulateOne(core, current_cycle, new_misses);
        //auto t2 = std::chrono::high_resolution_clock::now();
        //timer += std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();

        bool core_finished=false;
        bool running=true;

        if(new_misses.size()>0)
        {
            //FETCH MISSES ARE SERVICED WHETHER THERE IS A RAW OR NOT
            if(new_misses.front()->getType()==spike_model::L2Request::AccessType::FETCH)
            {
                uint64_t lapse=0;
                if(current_cycle-spike_model->getScheduler()->getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                {
                    lapse=current_cycle-spike_model->getScheduler()->getCurrentTick();
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
                        if(current_cycle-spike_model->getScheduler()->getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                        {
                            lapse=current_cycle-spike_model->getScheduler()->getCurrentTick();
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
            /*if(trace_)
            {
                logger_.logStall(getClock()->currentCycle(), id_);
            }*/
            
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
}

void SimulationOrchestrator::handleEvents()
{
    //GET NEXT EVENT
    if(next_event_tick==sparta::Scheduler::INDEFINITE)
    {
        next_event_tick=spike_model->getScheduler()->nextEventTick();
        //printf("NEXT updated to %lu\n", next_event_tick);
    }
   
    //HANDLE ALL THE EVENTS FOR THE CURRENT CYCLE
    if(next_event_tick==current_cycle)
    {
        uint64_t advance=next_event_tick-spike_model->getScheduler()->getCurrentTick()+1; //Add 1 because the  run method is not inclusive
        //spike_model->getScheduler()->run(advance, true);
        spike_model->runRaw(advance);
        //Now the spike_model->getScheduler() has fully executed as many ticks as spike (spike_model->getScheduler().getCurrentTick()==current_cycle+1 && spike_model->getScheduler().getElapsedTicks()==current_cycle)
        next_event_tick=spike_model->getScheduler()->nextEventTick();
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
                    if(current_cycle-spike_model->getScheduler()->getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                    {
                        lapse=current_cycle-spike_model->getScheduler()->getCurrentTick();
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
                    if(current_cycle-spike_model->getScheduler()->getCurrentTick()!=sparta::Scheduler::INDEFINITE)
                    {
                        lapse=current_cycle-spike_model->getScheduler()->getCurrentTick();
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
                can_run=spike->ackRegister(req, current_cycle);
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
                /*if(trace_)
                {
                    logger_.logResume(getClock()->currentCycle(), id_);
                }*/
            }
        }
    }
}

void SimulationOrchestrator::run()
{    
    //Simulation will not end until there are neither events nor instructions to simulate
    //Each iteration of the loop handles a cycle
    while(!spike_model->getScheduler()->isFinished() || !spike_finished)
    {
        //printf("Current %lu, next %lu\n", current_cycle, next_event_tick);
        
        simulateInstInActiveCores();
        handleEvents();
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
    
    //current_cycle--;
   
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
