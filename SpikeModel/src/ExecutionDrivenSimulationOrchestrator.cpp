#include "ExecutionDrivenSimulationOrchestrator.hpp"

ExecutionDrivenSimulationOrchestrator::ExecutionDrivenSimulationOrchestrator(std::shared_ptr<spike_model::SpikeWrapper>& spike, std::shared_ptr<SpikeModel>& spike_model, std::shared_ptr<spike_model::FullSystemSimulationEventManager>& request_manager, uint32_t num_cores, uint32_t num_threads_per_core, uint32_t thread_switch_latency, uint16_t num_mshrs_per_core, bool trace, bool l1_writeback, spike_model::DetailedNoC* detailed_noc):
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
    l1_writeback(l1_writeback),
    is_fetch(false),
    detailed_noc_(detailed_noc),
    booksim_has_packets_in_flight_(false),
    max_in_flight_l1_misses(num_mshrs_per_core),
    in_flight_requests_per_l1(num_cores/num_threads_per_core),
    mshr_stalls_per_core(num_cores)
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
    pending_mcpu_insn.resize(num_cores,NULL);
    pending_simfence.resize(num_cores,NULL);
    waiting_on_fetch.resize(num_cores,false);
    waiting_on_mshrs.resize(num_cores,false);
}

void ExecutionDrivenSimulationOrchestrator::saveReports()
{
    //PRINTS THE SPARTA STATISTICS
    spike_model->saveReports();

    uint64_t tot=0;
    for(uint16_t i=0;i<num_cores;i++)
    {
        printf("Core %d: \n\tsimulated %lu instructions\n\tStalled on mshrs %lu times.\n", i, simulated_instructions_per_core[i], mshr_stalls_per_core[i]);
        tot+=simulated_instructions_per_core[i];
    }
    printf("Total simulated instructions %lu\n", tot);
    
    memoryAccessLatencyReport(); 
}

ExecutionDrivenSimulationOrchestrator::~ExecutionDrivenSimulationOrchestrator()
{
    saveReports();
}

void ExecutionDrivenSimulationOrchestrator::memoryAccessLatencyReport()
{
    uint64_t num_l1_hits=spike->getNumL1DataHits();
    uint64_t num_mem_accesses=num_l1_hits+num_l2_accesses;
   
    double avg_mem_access_time=(1*((double)num_l1_hits/num_mem_accesses))+(avg_mem_access_time_l1_miss*((double)num_l2_accesses/num_mem_accesses));

    std::cout << "Average memory access time: " << avg_mem_access_time <<" cycles\n";

    double avg_arbiter_latency=spike_model->getAvgArbiterLatency();
    double avg_l2_latency=spike_model->getAvgL2Latency();
    double avg_noc_latency=spike_model->getAvgNoCLatency();
    double avg_llc_latency=spike_model->getAvgLLCLatency();
    double avg_memory_controller_latency=spike_model->getAvgMemoryControllerLatency();

    std::cout << "Average memory access time breakdown (for accesses that miss all along the memory hierarchy):\n";
    std::cout << "\tArbiter: " << avg_arbiter_latency << "\n";
    std::cout << "\tL2: " << avg_l2_latency << "\n";
    std::cout << "\tNoC: " << avg_noc_latency << "\n";
    std::cout << "\tLLC: " << avg_llc_latency << "\n";
    std::cout << "\tMemory controller: " << avg_memory_controller_latency << "\n";
    //std::cout << "The monitored section took " << timer <<" nanoseconds\n";
}

uint64_t ExecutionDrivenSimulationOrchestrator::spartaDelay(uint64_t cycle)
{
    return cycle-spike_model->getScheduler()->getCurrentTick();
}

void ExecutionDrivenSimulationOrchestrator::simulateInstInActiveCores()
{
    for(uint16_t i=0;i<runnable_cores.size();i++)
    {
        core_finished=false;
        current_core=runnable_cores[i];

        simulated_instructions_per_core[current_core]++;

        if(trace && simulated_instructions_per_core[current_core]%1000==0)
        {
           logger_->logKI(current_cycle, current_core); 
        }

        std::list<std::shared_ptr<spike_model::Event>> new_spike_events;

        is_fetch = false;

        bool success=false;

        //auto t1 = std::chrono::high_resolution_clock::now();
        success=spike->simulateOne(current_core, current_cycle, new_spike_events);
        //auto t2 = std::chrono::high_resolution_clock::now();
        //timer += std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();

        core_active=success;

        for(std::shared_ptr<spike_model::Event> e:new_spike_events)
        {
            e->handle(this);
        }

        bool hasFreeSlot= hasArbiterQueueFreeSlot(current_core);

        if(!core_active && !hasFreeSlot)
        {
            std::cout << "Fetch miss and Arbiter Queue full for " << current_core << " and " << current_cycle << std::endl;
        }
        else if(!hasFreeSlot)
        {
            stalled_cores_for_arbiter.insert(current_core);
        }

        if(!core_active || !hasFreeSlot)
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
                logger_->logStall(current_cycle, current_core, 0);
            }
        }
    }
}

void ExecutionDrivenSimulationOrchestrator::handleSpartaEvents()
{
    //GET NEXT EVENT
    next_event_tick=spike_model->getScheduler()->nextEventTick();

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

void ExecutionDrivenSimulationOrchestrator::scheduleArbiter()
{
    request_manager->scheduleArbiter();
}

bool ExecutionDrivenSimulationOrchestrator::hasArbiterQueueFreeSlot(uint16_t core)
{
    return request_manager->hasArbiterQueueFreeSlot(core);
}

bool ExecutionDrivenSimulationOrchestrator::hasMsgInArbiter()
{
    return request_manager->hasMsgInArbiter();
}

void ExecutionDrivenSimulationOrchestrator::run()
{
    //Each iteration of the loop handles a cycle
    //Simulation will end when there are neither pending events nor more instructions to simulate
    while(!spike_model->getScheduler()->isFinished() || !spike_finished || booksim_has_packets_in_flight_)
    {
        simulateInstInActiveCores();
        handleSpartaEvents();
        //auto t1 = std::chrono::high_resolution_clock::now();
        scheduleArbiter();
        std::set<uint16_t>::iterator it = stalled_cores_for_arbiter.begin();
        while(it!=stalled_cores_for_arbiter.end())
        {
            if(hasArbiterQueueFreeSlot(*it))
            {
                resumeCore(*it);
                it = stalled_cores_for_arbiter.erase(it);
            }
            else
                it++;
        }
        //auto t2 = std::chrono::high_resolution_clock::now();
        //timer += std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();
        next_event_tick=spike_model->getScheduler()->nextEventTick();

        // Execute one cycle of BookSim
        if(detailed_noc_ != NULL)
        {
            booksim_has_packets_in_flight_ = detailed_noc_->runBookSimCycles(1, current_cycle);
            // BookSim can retire a packet and introduce an event that must be executed before the cycle saved in next_event_tick
            next_event_tick=spike_model->getScheduler()->nextEventTick();
            //std::cout << booksim_has_packets_in_flight_ << " at " << current_cycle << std::endl;
        }
        selectRunnableThreads();

        //If there are no active cores, booksim must not be executed at next cycle and there is a pending event
        if(active_cores.size()==0 && !booksim_has_packets_in_flight_ && next_event_tick!=sparta::Scheduler::INDEFINITE && (next_event_tick-current_cycle)>1 && !hasMsgInArbiter())
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
    printf("!!)!)!)!)!)!)!)\n");
}

void ExecutionDrivenSimulationOrchestrator::selectRunnableThreads()
{
    //If active core is changed, mark the other active thread in RR fashion as runnable
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

void ExecutionDrivenSimulationOrchestrator::submitToSparta(std::shared_ptr<spike_model::CacheRequest> r)
{
    r->setTimestamp(current_cycle);

    //Submit writebacks or requests of other types that have not been already submitted
    if(l1_writeback)
    {
        if(r->getType()==spike_model::CacheRequest::AccessType::WRITEBACK ||
           in_flight_requests_per_l1[r->getCoreId()/num_threads_per_core].find(r->getAddress())==in_flight_requests_per_l1[r->getCoreId()/num_threads_per_core].end())
        {
            request_manager->putEvent(r);
        }

        if(r->getType()!=spike_model::CacheRequest::AccessType::WRITEBACK)
        {
            in_flight_requests_per_l1[r->getCoreId()/num_threads_per_core].insert(std::pair<uint64_t,std::shared_ptr<spike_model::CacheRequest>>(r->getAddress(), r));
        }
    }
    else
    {
        if(r->getType()==spike_model::CacheRequest::AccessType::WRITEBACK ||
           r->getType()==spike_model::CacheRequest::AccessType::STORE ||
           in_flight_requests_per_l1[r->getCoreId()/num_threads_per_core].find(r->getAddress())==in_flight_requests_per_l1[r->getCoreId()/num_threads_per_core].end())
        {
            request_manager->putEvent(r);
        }

        if(r->getType()!=spike_model::CacheRequest::AccessType::WRITEBACK && r->getType()!=spike_model::CacheRequest::AccessType::STORE)
        {
            in_flight_requests_per_l1[r->getCoreId()/num_threads_per_core].insert(std::pair<uint64_t,std::shared_ptr<spike_model::CacheRequest>>(r->getAddress(), r));
        }
    }
}

void ExecutionDrivenSimulationOrchestrator::submitToSparta(std::shared_ptr<spike_model::Event> r)
{
    request_manager->putEvent(r);
}

void ExecutionDrivenSimulationOrchestrator::resumeCore(uint64_t core)
{
    std::vector<uint16_t>::iterator it;

    //core should only be made active if there is space in the Arbiter Queue
    //This is a conservative approach. A performance efficient approach would be to activate the core and let it execute
    //instructions until it generates a packet for arbiter.
    if(hasArbiterQueueFreeSlot(core))
    {
        it=std::find(stalled_cores.begin(), stalled_cores.end(), core);

        //If the core was stalled, make it active again
        if (it != stalled_cores.end())
        {
            stalled_cores.erase(it);
            active_cores.push_back(core);
        }
    }
    else{
       stalled_cores_for_arbiter.insert(core);
    }
}

void ExecutionDrivenSimulationOrchestrator::handle(std::shared_ptr<spike_model::CacheRequest> r)
{
    if(!r->isServiced())
    {
        if(in_flight_requests_per_l1[current_core/num_threads_per_core].size()>=max_in_flight_l1_misses)
        {
            core_active=false;
            waiting_on_mshrs[current_core] = true;
            mshr_stalls_per_core[current_core]++;
            pending_misses_per_core[current_core].push_back(r);
        }
        else
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
    }
    else
    {
        uint16_t core=r->getCoreId();
        bool is_fetch=r->getType()==spike_model::CacheRequest::AccessType::FETCH;
        bool is_load=r->getType()==spike_model::CacheRequest::AccessType::LOAD;
        bool is_store=r->getType()==spike_model::CacheRequest::AccessType::STORE;

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
        
        //For write through caches, if store was a hit, the later load/store request on this block would also be a hit
        //If store was a miss, the later load request on this block also needs to be submitted to sparta
        //as inflight list would not keep store request
        //Stores may bring lines that are pending for a load
        if(is_load || (is_store && l1_writeback))
        {
            //Update average memory access time metrics
            avg_mem_access_time_l1_miss=avg_mem_access_time_l1_miss+((float)(current_cycle-r->getTimestamp())-avg_mem_access_time_l1_miss)/num_l2_accesses;
            num_l2_accesses++;

            std::multimap<uint64_t, std::shared_ptr<spike_model::CacheRequest>>::iterator it,it_start,it_end;

            it_start=in_flight_requests_per_l1[core/num_threads_per_core].lower_bound(r->getAddress());
            it_end=in_flight_requests_per_l1[core/num_threads_per_core].upper_bound(r->getAddress());

            for (it=it_start; it!=it_end; ++it)
            {
                std::shared_ptr<spike_model::CacheRequest> serviced_request=(*it).second;
                if(serviced_request->getType()==spike_model::CacheRequest::AccessType::LOAD)
                {
                    can_run=spike->ackRegister(serviced_request->getCoreId(), serviced_request->getDestinationRegType(), serviced_request->getDestinationRegId(), current_cycle);
                }
            }
        }

        if(l1_writeback)
        {
            if(r->getType() != spike_model::CacheRequest::AccessType::WRITEBACK)
                in_flight_requests_per_l1[core/num_threads_per_core].erase(r->getAddress());
        }
        else
        {
            if(r->getType() != spike_model::CacheRequest::AccessType::WRITEBACK &&
               r->getType() != spike_model::CacheRequest::AccessType::STORE)
                in_flight_requests_per_l1[core/num_threads_per_core].erase(r->getAddress());
        }

        if(waiting_on_mshrs[core])
        {
            submitPendingCacheRequests(core);
        }

        can_run = can_run && !waiting_on_fetch[core];
        if(can_run)
        {
            submitPendingOps(core);
        }


        //Reload the cache and generate a writeback
        if(l1_writeback)
        {
            if(r && r->getType()!=spike_model::CacheRequest::AccessType::WRITEBACK && !r->getBypassL1())
            {
                std::shared_ptr<spike_model::CacheRequest> wb=spike->serviceCacheRequest(r, current_cycle);
                if(wb!=nullptr)
                {
                    if(in_flight_requests_per_l1[core/num_threads_per_core].size()<max_in_flight_l1_misses)
                    {
                        handle(wb);
                    }
                    else
                    {
                        pending_misses_per_core[current_core].push_back(wb);
                    }
                }
            }
        }
        else
        {
            if(r && r->getType()!=spike_model::CacheRequest::AccessType::WRITEBACK && !r->getBypassL1()
                 && r->getType()!=spike_model::CacheRequest::AccessType::STORE)
            {
                std::shared_ptr<spike_model::CacheRequest> wb=spike->serviceCacheRequest(r, current_cycle);
                if(wb!=nullptr)
                {
                    if(in_flight_requests_per_l1[core/num_threads_per_core].size()<max_in_flight_l1_misses)
                    {
                        handle(wb);
                    }
                    else
                    {
                        pending_misses_per_core[current_core].push_back(wb);
                    }
                }
            }
        }
    
        //If the core was stalled on MSHRs and all the pending requests have been submitted
        if(waiting_on_mshrs[core] && pending_misses_per_core[core].size()==0)
        {
            waiting_on_mshrs[core]=false;
            resumeCore(core);
        }


        //If there are MSHRs available, the core is not in a barrier and either a RAW was serviced or MSHRs just became available
        if(can_run && !threads_in_barrier[core])
        {
            resumeCore(core);
            if(trace_)
            {
                logger_->logResumeWithAddress(current_cycle, core, r->getAddress());

                /*if(r->memoryAck())
                {
                    printf("In\n");
                    mc=r->getMemoryController()+1;
                    bank=r->getMemoryBank()+1;
                }
                else
                {
                        printf("Out\n");
                    }*/
                    logger_->logResumeWithMC(current_cycle, core, r->getMemoryController());
                    logger_->logResumeWithMemBank(current_cycle, core, r->getMemoryBank());
                    logger_->logResumeWithCacheBank(current_cycle, core, r->getCacheBank());
                    logger_->logResumeWithTile(current_cycle, core, r->getHomeTile());
                }
            }
        }
    }

void ExecutionDrivenSimulationOrchestrator::handle(std::shared_ptr<spike_model::Finish> f)
{
    core_active=false;
    core_finished=true;
}

void ExecutionDrivenSimulationOrchestrator::handle(std::shared_ptr<spike_model::Fence> f)
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

void ExecutionDrivenSimulationOrchestrator::runPendingSimfence(uint64_t core)
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
                    logger_->logResume(current_cycle, i);
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

void ExecutionDrivenSimulationOrchestrator::handle(std::shared_ptr<spike_model::MCPUSetVVL> r)
{
    if(!r->isServiced())
    {
        if(is_fetch == true)
        {
            pending_get_vec_len[current_core] = r;
        }
        else
        {
            submitToSparta(r);
        }
    }
    else
    {
        uint16_t core = r->getCoreId();
        spike->setVVL(core, r->getVVL());
        bool can_run=spike->ackRegister(r->getCoreId(), spike_model::Request::RegType::INTEGER,
                                       r->getDestinationRegId(), current_cycle);
        if(can_run && !threads_in_barrier[core])
        {
            resumeCore(core);
            if(trace_)
            {
                logger_->logResume(current_cycle, core);
            }
        }
    }
}

void ExecutionDrivenSimulationOrchestrator::handle(std::shared_ptr<spike_model::ScratchpadRequest> r)
{
    sparta_assert(r->isServiced());

    uint64_t core=r->getCoreId();

    bool can_run=spike->ackRegister(r->getCoreId(), r->getDestinationRegType(),
                                       r->getDestinationRegId(), current_cycle);
    if(can_run && !threads_in_barrier[core])
    {
        resumeCore(core);
        if(trace_)
        {
            logger_->logResume(current_cycle, core);
        }
    }
}

void ExecutionDrivenSimulationOrchestrator::handle(std::shared_ptr<spike_model::MCPUInstruction> i)
{
    if(is_fetch == true)
    {
        pending_mcpu_insn[current_core] = i;
    }
    else
    {
        submitToSparta(i);
    }
}

//latency and fetch
void ExecutionDrivenSimulationOrchestrator::handle(std::shared_ptr<spike_model::InsnLatencyEvent> r)
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
                logger_->logResume(current_cycle, r->getCoreId());
            }
        }
    }
}

void ExecutionDrivenSimulationOrchestrator::submitPendingCacheRequests(uint64_t core)
{
    while(pending_misses_per_core[core].size()>0 && in_flight_requests_per_l1[core/num_threads_per_core].size()<max_in_flight_l1_misses)
    {
        std::shared_ptr<spike_model::CacheRequest> miss=pending_misses_per_core[core].front();
        pending_misses_per_core[core].pop_front();

        //Update the timestamp to the current cycle, which is when the miss actually happens
        miss->setTimestamp(current_cycle);

        submitToSparta(miss);
    }

    if(pending_misses_per_core[core].size()>0)
    {
        waiting_on_mshrs[current_core] = true;
    }
}

void ExecutionDrivenSimulationOrchestrator::submitPendingOps(uint64_t core)
{
    if(pending_simfence[core] != NULL)
    {
        runPendingSimfence(core);
        pending_simfence[core] = NULL;
    }

    submitPendingCacheRequests(core);

    if(pending_get_vec_len[core] != NULL)
    {
        //Update the timestamp to the current cycle, which is when the request actually happens
        pending_get_vec_len[core]->setTimestamp(current_cycle);
        submitToSparta(pending_get_vec_len[core]);
        pending_get_vec_len[core] = NULL;
    }

    if(pending_mcpu_insn[core] != NULL)
    {
        //Update the timestamp to the current cycle, which is when the request actually happens
        pending_mcpu_insn[core]->setTimestamp(current_cycle);
        submitToSparta(pending_mcpu_insn[core]);
        pending_mcpu_insn[core] = NULL;
    }
}
