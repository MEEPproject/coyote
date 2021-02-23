#include "SimulationOrchestrator.hpp"

SimulationOrchestrator::SimulationOrchestrator(std::shared_ptr<spike_model::SpikeWrapper>& spike, std::shared_ptr<SpikeModel>& spike_model, std::shared_ptr<spike_model::RequestManagerIF>& request_manager, uint32_t num_cores, uint32_t num_threads_per_core, uint32_t thread_switch_latency, bool trace):
    spike(spike),
    spike_model(spike_model),
    request_manager(request_manager),
    num_cores(num_cores),
    num_threads_per_core(num_threads_per_core),
    thread_switch_latency(thread_switch_latency),
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

    for(uint16_t i=0;i<num_cores;i+=num_threads_per_core)
    {
        runnable_cores.push_back(i);
        runnable_after.push_back(0);
    }

    thread_barrier_cnt = 0;
    threads_in_barrier.resize(num_cores,false);
}
        
uint64_t SimulationOrchestrator::spartaDelay(uint64_t cycle)
{
    return cycle-spike_model->getScheduler()->getCurrentTick();
}

void SimulationOrchestrator::simulateInstInActiveCores()
{
    for(uint16_t i=0;i<runnable_cores.size();i++)
    {
        core_finished=false;
        current_core=runnable_cores[i];

        simulated_instructions_per_core[current_core]++;

        if(trace && simulated_instructions_per_core[current_core]%1000==0)
        {
           logger_.logKI(current_cycle, current_core); 
        }

        std::list<std::shared_ptr<spike_model::SpikeEvent>> new_spike_events;
        //auto t1 = std::chrono::high_resolution_clock::now();
        bool success=spike->simulateOne(current_core, current_cycle, new_spike_events);
        //auto t2 = std::chrono::high_resolution_clock::now();
        //timer += std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();

        core_active=success;

        for(std::shared_ptr<spike_model::SpikeEvent> e:new_spike_events)
        {
            e->handle(this);
        }

        if(!core_active)
        {
            std::vector<uint16_t>::iterator itr;
            itr = std::find(active_cores.begin(), active_cores.end(), current_core);
            if(itr != active_cores.end())
            {
                active_cores.erase(itr);
            }

            runnable_cores.erase(runnable_cores.begin() + i);
            cur_cycle_suspended_threads.push_back(current_core);
            runnable_after[current_core/num_threads_per_core] = current_cycle + thread_switch_latency;

            i--; //We have deleted an element, so we have to update the index that we are using to traverse the data structure

            if(!core_finished)
            {
                //The core is not active and is not finished, so it goe into the stalled cores list
                stalled_cores.push_back(current_core);
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
            if(trace_)
            {
                logger_.logStall(current_cycle, current_core, 0);
            }
        }
    }
}

void SimulationOrchestrator::handleSpartaEvents()
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

            if(can_run && !threads_in_barrier[core])
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
        handleSpartaEvents();
        selectRunnableThreads();
    
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

void SimulationOrchestrator::selectRunnableThreads()
{
    //If active core is ehanged, mark the other active thread in RR fashion as runnable
    for(uint16_t i = 0; i < cur_cycle_suspended_threads.size(); i++)
    {
        //Make runnable, the next active thread from the group
        uint16_t core = cur_cycle_suspended_threads[i];
        uint16_t core_group = core / num_threads_per_core;
        if(current_cycle >= runnable_after[core_group])
        {
            uint16_t start_core_id = core_group * num_threads_per_core;
            uint32_t cntr = 1;
            while(cntr <= num_threads_per_core)
            {
                uint16_t next_thread_id = start_core_id + ((core + cntr) % num_threads_per_core);
                if(std::find(active_cores.begin(), active_cores.end(), 
                            next_thread_id) != active_cores.end())
                {
                    runnable_cores.push_back(next_thread_id);
                    cur_cycle_suspended_threads.erase(cur_cycle_suspended_threads.begin() + i);
                    i--;
                    break;
                }
                cntr++;
            }
        }
    }
}


void SimulationOrchestrator::handle(std::shared_ptr<spike_model::Request> r)
{
    if(r->getType()==spike_model::Request::AccessType::FETCH)
    {
        //Fetch misses are serviced whether there is a raw or not. Subsequent, misses will be submitted later.
        submitToSparta(r);
        core_active=false;
    }
    else if(core_active)
    {
        if(r->getType()!=spike_model::Request::AccessType::WRITEBACK)
        {
            submitToSparta(r);
        }
        else //WRITEBACKS ARE HANDLED LAST
        {
            pending_writebacks_per_core[current_core].push_back(r);
        }
    }
    else //A core will be stalled if a RAW or fetch miss is detected
    {
        pending_misses_per_core[current_core].push_back(r); //Instructions are not replayed, so we have to store the misses of a raw or under a fetch, so they are serviced later
    }
}


void SimulationOrchestrator::submitToSparta(std::shared_ptr<spike_model::Request> r)
{
    uint64_t lapse=1;
    if(current_cycle-spike_model->getScheduler()->getCurrentTick()!=sparta::Scheduler::INDEFINITE)
    {
       lapse=lapse+spartaDelay(current_cycle);
    }
    request_manager->putRequest(r, lapse);
}

void SimulationOrchestrator::handle(std::shared_ptr<spike_model::Finish> f)
{
    core_active=false;
    core_finished=true;
}


void SimulationOrchestrator::handle(std::shared_ptr<spike_model::Fence> f)
{
    //set the thread_barrier_cnt to number of threads, if not already set
    if(thread_barrier_cnt == 0)
    {
        thread_barrier_cnt = num_cores - 1 ;
        std::cout << " barrier cnt " << thread_barrier_cnt << std::endl;
        threads_in_barrier[current_core] = true;
        core_active=false;
        std::cout << "first core " << current_core
                  << " waiting for barrier " << current_cycle << std::endl;
    }
    else if(thread_barrier_cnt == 1)
    {
        //last thread arrived
        thread_barrier_cnt = 0;
        for(uint32_t i = 0; i < num_cores;i++)
        {
            if(i != current_core)
            {
                active_cores.push_back(i);
            }
            threads_in_barrier[i] = false;
        }

        stalled_cores.clear();
        uint16_t my_core_gp = current_core/num_threads_per_core;

        //Mark the core for which the threads need to be make runnable
        for(uint32_t i = 0; i < num_cores;i+=num_threads_per_core)
        {
            if(i != my_core_gp)
            {
                runnable_cores.push_back(i);
            }
        }
        cur_cycle_suspended_threads.clear();
        std::cout << "Last Core " << current_core << " reached barrier " << current_cycle << std::endl;
    }
    else
    {
        thread_barrier_cnt--;
        threads_in_barrier[current_core] = true;
        core_active=false;
        std::cout << "Core " << current_core << " waiting for barrier " << current_cycle << std::endl;
    }
}
