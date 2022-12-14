// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 


#ifndef __EXECUTION_DRIVEN_SIMULATION_ORCHESTRATOR_HH__
#define __EXECUTION_DRIVEN_SIMULATION_ORCHESTRATOR_HH__
    
#include <memory>
#include <set>
#include <map>

#include "spike_wrapper.h"
#include "FullSystemSimulationEventManager.hpp"
#include "Coyote.hpp"
#include "Event.hpp"
#include "EventVisitor.hpp"
#include "CacheRequest.hpp"
#include "ScratchpadRequest.hpp"
#include "MemoryTile/MCPUSetVVL.hpp"
#include "MemoryTile/MCPUInstruction.hpp"
#include "Finish.hpp"
#include "VectorWaitingForScalarStore.hpp"
#include "LogCapable.hpp"
#include "SimulationOrchestrator.hpp"
#include "StallReason.hpp"
#include "NoC/NoC.hpp"

class ExecutionDrivenSimulationOrchestrator : public SimulationOrchestrator, public coyote::EventVisitor
{
    using coyote::EventVisitor::handle; //This prevents the compiler from warning on overloading 

    /**
     * \class ExecutionDrivenSimulationOrchestrator
     *
     * \brief ExecutionDrivenSimulationOrchestrator is the glue that puts together Spike and Sparta in Coyote.
     *
     * Its purpose is to handle the execution of instructions on Spike, the forwarding of L2 Requests
     * to Sparta and the notifications to Sparta when a request has been serviced.
     *
     */

    public:
        /*!
         * \brief Constructor for the ExecutionDrivenSimulationOrchestrator
         * \param spike An instance of a wrapped Spike simulation
         * \param coyote The model for the architecture that will be simulated
         * \param request_manager The request manager to communicate requests to the modelled architecture
         * \param num_cores The number of simukated cores
         * \param num_threads_per_core The numbers of thread that will run in the same core in CGMT
         * \param thread_switch_latency The penalty of switching between threads in CGMT
         * \param num_mshrs_per_core The number of Miss Status Holding Registers per core
         * \param trace Whether tracing is enabled or not
         * \param l1_writeback Whether l1 is writeback or writethrough
         * \param noc A pointer to the simulated NoC
         */
        ExecutionDrivenSimulationOrchestrator(std::shared_ptr<coyote::SpikeWrapper>& spike, std::shared_ptr<Coyote>& coyote, std::shared_ptr<coyote::FullSystemSimulationEventManager>& request_manager, uint32_t num_cores, uint32_t num_threads_per_core, uint32_t thread_switch_latency, uint16_t num_mshrs_per_core, bool trace, bool l1_writeback, coyote::NoC* noc);

        /*!
         * \brief Destructor for ExecutionDrivenSimulationOrchestrator
         */
        ~ExecutionDrivenSimulationOrchestrator();

        /*!
         * \brief Triggers the simulation
         */
        virtual void run() override;

         /*!
         * \brief Handles a cache request
         * \param r The event to handle
         * \note This function assumes that, for each cycle and core, if a fetch miss has happened it will be handled first.
         */
        virtual void handle(std::shared_ptr<coyote::CacheRequest> r) override;

        /*!
         * \brief Handles a finish event
         * \param f The finish event to handle
         */
        virtual void handle(std::shared_ptr<coyote::Finish> f) override;


        /*!
         * \brief Handles a finish event
         * \param f The fence event to handle
         */
        void handle(std::shared_ptr<coyote::Fence> f) override;
    
        void handle(std::shared_ptr<coyote::VectorWaitingForScalarStore> e) override;

        void handle(std::shared_ptr<coyote::MCPUSetVVL> r) override;
        
                
        /*!
         * \brief Handles an MCPUInstruction
         * \param i The instruction to handle
         */
        void handle(std::shared_ptr<coyote::MCPUInstruction> i) override;

        /*!
         * \brief Handles a scratchpad event
         * \param r The scratchpad event to handle
         */
        void handle(std::shared_ptr<coyote::ScratchpadRequest> r) override;
        
        /*!
         * \brief Handles an instruction latency event
         * \param r The instruction latency event to handle
         */
        void handle(std::shared_ptr<coyote::InsnLatencyEvent> r) override;

        /*!
         * \brief Show the statistics of the simulation
         */
        virtual void saveReports() override;

    private:

        std::shared_ptr<coyote::SpikeWrapper> spike;
        std::shared_ptr<Coyote> coyote;
        std::shared_ptr<coyote::FullSystemSimulationEventManager> request_manager;
        uint32_t num_cores;
        uint32_t num_threads_per_core;
        uint32_t thread_switch_latency;


        std::vector<uint16_t> active_cores;
        std::vector<uint16_t> stalled_cores;
        std::vector<uint16_t> runnable_cores;
        std::vector<bool> waiting_on_fetch;
        std::vector<bool> waiting_on_mshrs;
        std::vector<bool> waiting_on_scalar_stores;
        std::vector<uint64_t> runnable_after;
        std::vector<uint16_t> cur_cycle_suspended_threads;
        std::vector<bool> threads_in_barrier;

        std::vector<std::list<std::shared_ptr<coyote::CacheRequest>>> pending_misses_per_core; //(num_cores);
        std::vector<std::shared_ptr<coyote::MCPUSetVVL>> pending_get_vec_len; //(num_cores);
        std::vector<std::shared_ptr<coyote::MCPUInstruction>> pending_mcpu_insn; //(num_cores);
        std::vector<std::shared_ptr<coyote::Fence>> pending_simfence; //(num_cores);
        std::vector<uint64_t> simulated_instructions_per_core; //(num_cores);
        std::vector<std::list<std::shared_ptr<coyote::InsnLatencyEvent>>> pending_insn_latency_event; //(num_cores);

        uint64_t thread_barrier_cnt;
        uint64_t current_cycle;
        uint64_t next_event_tick;
        uint64_t timer;        
        bool spike_finished;

        uint32_t current_core;
        bool core_active;
        bool core_finished;
        StallReason stall_reason;

        bool trace;
        bool l1_writeback;

        bool is_fetch;

        coyote::NoC* noc_;    //! Pointer to the NoC
        bool noc_has_packets_in_flight_;        //! Flag that indicates if the noc has packets in flight
        std::set<uint16_t> stalled_cores_for_arbiter;
        
        uint16_t max_in_flight_l1_misses;
        std::vector<std::multimap<uint64_t,std::shared_ptr<coyote::CacheRequest>>> in_flight_requests_per_l1;
        uint16_t available_mshrs;
        std::vector<uint64_t> mshr_stalls_per_core; //(num_cores);

        float avg_mem_access_time_l1_miss=0;
        float avg_time_to_reach_l2=0;
        uint64_t num_l2_accesses=1; //Initialized to one to calcullate a rolling average

        uint16_t submittedCacheRequestsInThisCycle;

        /*!
         * \brief Simulate an instruction in each of the active cores
         */
        void simulateInstInActiveCores();
        
        /*!
         * \brief Handle the events for cycles earlier or equal to the current cycle
         */
        void handleSpartaEvents();

        /*!
         * \brief Selects the runnable threads for the next cycle
         */
        void selectRunnableThreads();

        /*!
         * \brief Calcullate the difference between a cycle value and Sparta time.
         * \param cycle The value that will be compared to the Sparta clock.
         * \return The difference between cycle and the value in the Sparta clock in cycles.
         */
        uint64_t spartaDelay(uint64_t cycle);

        /*!
         * \brief Submit an event to Sparta.
         * \param r The event to submit
         */
        void submitToSparta(std::shared_ptr<coyote::Event> r);

        /*!
         * \brief Submit a cache request to Sparta.
         * \param r The request to submit
         * \note This is equivalente to submitToSparta(std::shared_ptr<coyote::Event> r), but it updates the number of in flight L1 misses
         */
        void submitToSparta(std::shared_ptr<coyote::CacheRequest> r);

        /*
         * \brief Resume simulation on a core that is stalled
         * \param core The id of the core that will resume simulation
         */
        bool resumeCore(uint64_t core);

        /*
         * \brief Submit the pending operations of any kind to sparta
         * \param core The id of the core that will submit the operations
         */
        void submitPendingOps(uint64_t core);

        /*
         * \brief Submit the pending cache requests to sparta
         * \param core The id of the core that will submit the cache requests
         */
        void submitPendingCacheRequests(uint64_t core);
        
         /*
         * \brief run the pending simfence operation
         * \param core The id of the core that will run the simfence
         */
        void runPendingSimfence(uint64_t core);

        void scheduleArbiter();

        bool hasMsgInArbiter();

        bool hasArbiterQueueFreeSlot(uint16_t core);

        void memoryAccessLatencyReport();
};
#endif
