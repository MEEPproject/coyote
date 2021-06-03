#include "SimulationOrchestrator.hpp"

SimulationOrchestrator::SimulationOrchestrator(std::shared_ptr<spike_model::SpikeWrapper>& spike, std::shared_ptr<SpikeModel>& spike_model, std::shared_ptr<spike_model::EventManager>& request_manager, uint32_t num_cores, uint32_t num_threads_per_core, uint32_t thread_switch_latency, bool trace, spike_model::DetailedNoC* detailed_noc):
    spike(spike),
    spike_model(spike_model),
    request_manager(request_manager),
    num_cores(num_cores),
    num_threads_per_core(num_threads_per_core),
    thread_switch_latency(thread_switch_latency),
    pending_misses_per_core(num_cores),
    simulated_instructions_per_core(num_cores),
    pending_insn_latency_event(num_cores),
    current_cycle(1),
    next_event_tick(sparta::Scheduler::INDEFINITE),
    timer(0),
    spike_finished(false),
    trace(trace),
    is_fetch(false),
    detailed_noc_(detailed_noc)
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
    pending_get_vec_len.resize(num_cores,NULL);
    pending_simfence.resize(num_cores,NULL);
    waiting_on_fetch.resize(num_cores,false);
    waiting_on_mshrs.resize(num_cores,false);
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

        std::list<std::shared_ptr<spike_model::Event>> new_spike_events;

        is_fetch = false;

        bool success=false;

        //Stall the core if there are no remaining MSHRs. This is conservative. Actually the core may run until the next miss load happens
        if(spike->checkNumInFlightL1Misses(current_core)<max_in_flight_l1_misses)
        {
            //auto t1 = std::chrono::high_resolution_clock::now();
            success=spike->simulateOne(current_core, current_cycle, new_spike_events);
            //auto t2 = std::chrono::high_resolution_clock::now();
            //timer += std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();
        }
        else
        {
            waiting_on_mshrs[current_core] = true;
        }
        core_active=success;

        for(std::shared_ptr<spike_model::Event> e:new_spike_events)
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
        std::exception_ptr eptr;
        try
        {
            spike_model->runRaw(advance);
        } 
        catch (...) 
        {
            eptr = std::current_exception();
        }
        
        if(eptr != std::exception_ptr())
        {
            std::cerr << SPARTA_CMDLINE_COLOR_ERROR "Exception while running" SPARTA_CMDLINE_COLOR_NORMAL
                      << std::endl;
            try 
            {
                std::rethrow_exception(eptr);
            }
            catch(const std::exception &e) 
            {
                std::cerr << e.what() << std::endl;
                eptr =  std::current_exception();
            }
        }

        // Rethrow exception if necessary
        if(eptr != std::exception_ptr())
        {
            std::rethrow_exception(eptr);
        }


        //Now the Sparta Scheduler and the Orchestrator are in sync
        next_event_tick=spike_model->getScheduler()->nextEventTick();

        //Check serviced requests
        while(request_manager->hasServicedRequest())
        {
            request_manager->getServicedRequest()->handle(this);
        }
    }
}

void SimulationOrchestrator::run()
{
    //Each iteration of the loop handles a cycle
    //Simulation will end when there are neither pending events nor more instructions to simulate
    while(!spike_model->getScheduler()->isFinished() || !spike_finished)
    {
        //printf("---Current %lu, next %lu. Bools: %lu, %lu. Insts: %lu\n", current_cycle, next_event_tick, active_cores.size(), stalled_cores.size(), simulated_instructions_per_core[0]);

        simulateInstInActiveCores();
        handleSpartaEvents();
        bool booksim_at_next_cycle = false;
        // Execute one cycle of BookSim
        if(detailed_noc_ != NULL)
        {
            booksim_at_next_cycle = detailed_noc_->runBookSimCycles(1, current_cycle);
            // BookSim can retire a packet and introduce an event that must be executed before the cycle saved in next_event_tick
            next_event_tick=spike_model->getScheduler()->nextEventTick();
        }
        selectRunnableThreads();
    
        //If there are no active cores, booksim must not be executed at next cycle and there is a pending event
        if(active_cores.size()==0 && !booksim_at_next_cycle && next_event_tick!=sparta::Scheduler::INDEFINITE && (next_event_tick-current_cycle)>1)
        {
            // Advance BookSim clock
            if(detailed_noc_ != NULL)
                detailed_noc_->runBookSimCycles(next_event_tick-current_cycle-1, current_cycle); // -1 is because current cycle was executed above
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
        //Make runnable, the next active thread from the group if the core has available MSHRs
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

void SimulationOrchestrator::submitToSparta(std::shared_ptr<spike_model::Event> r)
{
    request_manager->putRequest(r);
}

void SimulationOrchestrator::resumeCore(uint64_t core)
{
    std::vector<uint16_t>::iterator it;

    it=std::find(stalled_cores.begin(), stalled_cores.end(), core);

    //If the core was stalled, make it active again
    if (it != stalled_cores.end())
    {
        stalled_cores.erase(it);
        active_cores.push_back(core);
    }
}

void SimulationOrchestrator::handle(std::shared_ptr<spike_model::CacheRequest> r)
{
    if(!r->isServiced())
    {
        if(r->getType()==spike_model::CacheRequest::AccessType::FETCH)
        {
            //Fetch misses are serviced. Subsequent, misses will be submitted later.
            submitToSparta(r);
            core_active=false;
            is_fetch = true;
            waiting_on_fetch[current_core] = true;
        }
        else if(core_active)
        {
            submitToSparta(r);
        }
        else //A core will be stalled if fetch miss is detected
        {
            pending_misses_per_core[current_core].push_back(r); //Instructions are not replayed, so we have to store the misses under a fetch, so they are serviced later
        }
    }
    else
    {
        uint16_t core=r->getCoreId();

        bool is_fetch=r->getType()==spike_model::CacheRequest::AccessType::FETCH;
        bool is_load=r->getType()==spike_model::CacheRequest::AccessType::LOAD;
        bool is_store=r->getType()==spike_model::CacheRequest::AccessType::STORE;

        size_t prev_in_flight=0;
        bool can_run=false;

        //TODO: This could be a switch
        if(is_fetch)
        {
            can_run = true;
            waiting_on_fetch[core] = false;

            /*If current cycle is greater than the timestamp when the requested regid
              is set, there is no need to generate any event as the register value is
              already available.
            */
            while(pending_insn_latency_event[core].size()>0)
            {
                std::shared_ptr<spike_model::InsnLatencyEvent> latency_evt =
                               pending_insn_latency_event[core].front();
                pending_insn_latency_event[core].pop_front();
                if(current_cycle < latency_evt->getAvailCycle())
                {
                    //Update the timestamp to the current cycle, which is when the
                    //request actually happens
                    latency_evt->setTimestamp(current_cycle);
                    submitToSparta(latency_evt);
                    can_run = false;
                }
                else
                {
                    //clear the pending register
                    spike->canResume(latency_evt->getCoreId(),
                                     latency_evt->getSrcRegId(),
                                     latency_evt->getSrcRegType(),
                                     latency_evt->getDestinationRegId(),
                                     latency_evt->getDestinationRegType(),
                                     latency_evt->getInsnLatency(), current_cycle);
                }
            }
        }


        if(is_load)
        {
            can_run=spike->ackRegister(r, current_cycle);
        }
        
        if(is_load || is_store)
        {
            prev_in_flight=spike->checkNumInFlightL1Misses(core);
        }
        

        can_run = can_run && !waiting_on_fetch[core];

        if(can_run)
        {
            submitPendingOps(core);
        }


        //Reload the cache and generate a writeback
        if(r->getType()!=spike_model::CacheRequest::AccessType::WRITEBACK)
        {
            std::shared_ptr<spike_model::CacheRequest> wb=spike->serviceCacheRequest(r, current_cycle);
            if(wb!=nullptr)
            {
                submitToSparta(wb);
            }
        }

        //If MSHRs became available
        if(prev_in_flight>=max_in_flight_l1_misses && spike->checkNumInFlightL1Misses(core)<max_in_flight_l1_misses)
        {
            //resume all the cores in the group that were waiting for MSHRs
            uint16_t core_group_start=(core/num_threads_per_core)*num_threads_per_core;
            uint16_t core_group_end=core_group_start+num_threads_per_core;
            for(uint16_t i = core_group_start; i < core_group_end; i++)
            {
                if(waiting_on_mshrs[i])
                {
                    waiting_on_mshrs[i]=false;
                    resumeCore(i);
                }
            }
        }


        //If there are MSHRs available, the core is not in a barrier and either a RAW was serviced or MSHRs just became available
        if(can_run && !threads_in_barrier[core])
        {
            resumeCore(core);
            if(trace_)
            {
                logger_.logResumeWithAddress(current_cycle, core, r->getAddress());
            }
        }
    }
}

void SimulationOrchestrator::handle(std::shared_ptr<spike_model::Finish> f)
{
    core_active=false;
    core_finished=true;
}

void SimulationOrchestrator::handle(std::shared_ptr<spike_model::Fence> f)
{
    if(is_fetch)
    {
        pending_simfence[current_core] = f;
    }
    else
    {
        runPendingSimfence(current_core);
    }
}

void SimulationOrchestrator::runPendingSimfence(uint64_t core)
{
    //set the thread_barrier_cnt to number of threads, if not already set
    if(thread_barrier_cnt == 0)
    {
        thread_barrier_cnt = num_cores - 1 ;
        if(thread_barrier_cnt != 0) //ensure that more than one cores are getting simulated
        {
            std::cout << " barrier cnt " << thread_barrier_cnt << std::endl;
            threads_in_barrier[core] = true;
            core_active=false;
            std::cout << "first core " << core
                  << " waiting for barrier " << current_cycle << std::endl;
        }
    }
    else if(thread_barrier_cnt == 1)
    {
        //last thread arrived
        thread_barrier_cnt = 0;
        uint64_t my_core_gp = core/num_threads_per_core;
        for(uint32_t i = 0; i < num_cores;i++)
        {
            std::vector<uint16_t>::iterator it;
            it=std::find(stalled_cores.begin(), stalled_cores.end(), i);
            if(it != stalled_cores.end())  //Check if i is not already in active core
            {
                resumeCore(i);
                if(trace_)
                {
                    logger_.logResume(current_cycle, i);
                }
                //The below condition makes sure that the thread from the core-group
                //in which 'core' belongs, is also made active
                if(i == core)
                  my_core_gp = std::numeric_limits<uint64_t>::max();
            }
            threads_in_barrier[i] = false;
        }

        stalled_cores.clear();

        //Mark the core for which the threads need to be make runnable
        for(uint32_t i = 0; i < num_cores;i+=num_threads_per_core)
        {
            uint16_t curr_core_gp = i / num_threads_per_core;
            if(curr_core_gp != my_core_gp)
            {
                runnable_cores.push_back(i);
            }
        }
        cur_cycle_suspended_threads.clear();
        std::cout << "Last Core " << core << " reached barrier " << current_cycle << std::endl;
    }
    else
    {
        thread_barrier_cnt--;
        threads_in_barrier[core] = true;
        core_active=false;
        std::cout << "Core " << core << " waiting for barrier " << current_cycle << std::endl;
    }
}

void SimulationOrchestrator::handle(std::shared_ptr<spike_model::MCPUSetVVL> r)
{
    if(is_fetch == true) //RAW miss and a Fetch miss
    {
        //Fetch miss could be handled by this. RAW miss cannot
        pending_get_vec_len[current_core] = r;
    }
    else
    {
        submitToSparta(r);
    }
}

void SimulationOrchestrator::handle(std::shared_ptr<spike_model::ScratchpadRequest> r)
{
    sparta_assert(r->isServiced());

    uint64_t core=r->getCoreId();

    bool can_run=spike->ackRegister(r, current_cycle);
    if(can_run && !threads_in_barrier[core])
    {
        resumeCore(core);
        if(trace_)
        {
            logger_.logResume(current_cycle, core);
        }
    }
}
        
void SimulationOrchestrator::handle(std::shared_ptr<spike_model::MCPUInstruction> i)
{
    submitToSparta(i);
}

//latency and fetch
void SimulationOrchestrator::handle(std::shared_ptr<spike_model::InsnLatencyEvent> r)
{
    if(!r->isServiced())
    {
        if(is_fetch)
        {
            pending_insn_latency_event[current_core].push_back(r);
        }
        else
        {
            r->setTimestamp(current_cycle);
            submitToSparta(r);
        }
    }
    else
    {
        if(spike->canResume(r->getCoreId(), r->getSrcRegId(), r->getSrcRegType(),
                            r->getDestinationRegId(), r->getDestinationRegType(),
                            r->getInsnLatency(), current_cycle))
        {
            uint16_t core = r->getCoreId();
            submitPendingOps(core);
            resumeCore(core);
            if(trace_)
            {
                logger_.logResume(current_cycle, r->getCoreId());
            }
        }
    }
}

void SimulationOrchestrator::submitPendingOps(uint64_t core)
{
    if(pending_simfence[core] != NULL)
    {
        runPendingSimfence(core);
        pending_simfence[core] = NULL;
    }

    while(pending_misses_per_core[core].size()>0)
    {
        std::shared_ptr<spike_model::CacheRequest> miss=pending_misses_per_core[core].front();
        pending_misses_per_core[core].pop_front();

        //Update the timestamp to the current cycle, which is when the miss actually happens
        miss->setTimestamp(current_cycle);
        submitToSparta(miss);
    }

    if(pending_get_vec_len[core] != NULL)
    {
        //Update the timestamp to the current cycle, which is when the request actually happens
        pending_get_vec_len[core]->setTimestamp(current_cycle);
        submitToSparta(pending_get_vec_len[core]);
        pending_get_vec_len[core] = NULL;
    }
}
