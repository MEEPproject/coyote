
#ifndef __EXECUTION_DRIVEN_SIMULATION_ORCHESTRATOR_HH__
#define __EXECUTION_DRIVEN_SIMULATION_ORCHESTRATOR_HH__
    
#include <memory>
#include "spike_wrapper.h"
#include "FullSystemSimulationEventManager.hpp"
#include "SpikeModel.hpp"
#include "Event.hpp"
#include "EventVisitor.hpp"
#include "CacheRequest.hpp"
#include "ScratchpadRequest.hpp"
#include "MemoryTile/MCPUSetVVL.hpp"
#include "MemoryTile/MCPUInstruction.hpp"
#include "Finish.hpp"
#include "LogCapable.hpp"
#include "SimulationOrchestrator.hpp"
#include "NoC/DetailedNoC.hpp"
#include <set>
#include <map>

class ExecutionDrivenSimulationOrchestrator : public SimulationOrchestrator, public spike_model::EventVisitor
{
    using spike_model::EventVisitor::handle; //This prevents the compiler from warning on overloading 

    /**
     * \class ExecutionDrivenSimulationOrchestrator
     *
     * \brief ExecutionDrivenSimulationOrchestrator is the glue that puts together  Spike and Sparta in Coyote.
     *
     * Its purpose is to handle the execution of instructions on Spike, the forwarding of L2 Requests
     * to Sparta and the notifications to Sparta when a request has been serviced.
     *
     */

    public:

        /*!
         * \brief Constructor for the ExecutionDrivenSimulationOrchestrator
         * \param spike An instance of a wrapped Spike simulation
         * \param spike_model The model for the architecture that will be simulated
         * \param request_manager The request manager to communicate requests to the modelled architecture
         * \param num_cores The number of simukated cores
         * \param num_threads_per_core The numbers of thread that will run in the same core in CGMT
         * \param thread_switch_latency The penalty of switching between threads in CGMT
         * \param num_mshrs_per_core The number of Miss Status Holding Registers per core
         * \param trace Whether tracing is enabled or not
         * \param l1_writeback Whether l1 is writeback or writethrough
         * \param detailed_noc A pointer to the simulated DetailedNoC or NULL if a detailed model is not used
         */
        ExecutionDrivenSimulationOrchestrator(std::shared_ptr<spike_model::SpikeWrapper>& spike, std::shared_ptr<SpikeModel>& spike_model, std::shared_ptr<spike_model::FullSystemSimulationEventManager>& request_manager, uint32_t num_cores, uint32_t num_threads_per_core, uint32_t thread_switch_latency, uint16_t num_mshrs_per_core, bool trace, bool l1_writeback, spike_model::DetailedNoC* detailed_noc);

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
        virtual void handle(std::shared_ptr<spike_model::CacheRequest> r) override;

        /*!
         * \brief Handles a finish event
         * \param f The finish event to handle
         */
        virtual void handle(std::shared_ptr<spike_model::Finish> f) override;


        /*!
         * \brief Handles a finish event
         * \param f The fence event to handle
         */
        void handle(std::shared_ptr<spike_model::Fence> f) override;

        void handle(std::shared_ptr<spike_model::MCPUSetVVL> r) override;
        
                
        /*!
         * \brief Handles an MCPUInstruction
         * \param i The instruction to handle
         */
        void handle(std::shared_ptr<spike_model::MCPUInstruction> i) override;

        /*!
         * \brief Handles a scratchpad event
         * \param r The scratchpad event to handle
         */
        void handle(std::shared_ptr<spike_model::ScratchpadRequest> r) override;
        
        /*!
         * \brief Handles an instruction latency event
         * \param r The instruction latency event to handle
         */
        void handle(std::shared_ptr<spike_model::InsnLatencyEvent> r) override;

        /*!
         * \brief Show the statistics of the simulation
         */
        virtual void saveReports() override;

    private:
        std::shared_ptr<spike_model::SpikeWrapper> spike;
        std::shared_ptr<SpikeModel> spike_model;
        std::shared_ptr<spike_model::FullSystemSimulationEventManager> request_manager;
        uint32_t num_cores;
        uint32_t num_threads_per_core;
        uint32_t thread_switch_latency;


        std::vector<uint16_t> active_cores;
        std::vector<uint16_t> stalled_cores;
        std::vector<uint16_t> runnable_cores;
        std::vector<bool> waiting_on_fetch;
        std::vector<bool> waiting_on_mshrs;
        std::vector<uint64_t> runnable_after;
        std::vector<uint16_t> cur_cycle_suspended_threads;
        std::vector<bool> threads_in_barrier;

        std::vector<std::list<std::shared_ptr<spike_model::CacheRequest>>> pending_misses_per_core; //(num_cores);
        std::vector<std::shared_ptr<spike_model::MCPUSetVVL>> pending_get_vec_len; //(num_cores);
        std::vector<std::shared_ptr<spike_model::MCPUInstruction>> pending_mcpu_insn; //(num_cores);
        std::vector<std::shared_ptr<spike_model::Fence>> pending_simfence; //(num_cores);
        std::vector<uint64_t> simulated_instructions_per_core; //(num_cores);
        std::vector<std::list<std::shared_ptr<spike_model::InsnLatencyEvent>>> pending_insn_latency_event; //(num_cores);

        uint64_t thread_barrier_cnt;
        uint64_t current_cycle;
        uint64_t next_event_tick;
        uint64_t timer;        
        bool spike_finished;

        uint32_t current_core;
        bool core_active;
        bool core_finished;

        bool trace;
        bool l1_writeback;

        bool is_fetch;

        spike_model::DetailedNoC* detailed_noc_;    //! Pointer to the NoC
        bool booksim_has_packets_in_flight_;        //! Flag that indicates if booksim has packets in flight
        std::set<uint16_t> stalled_cores_for_arbiter;
        
        uint16_t max_in_flight_l1_misses;
        std::vector<std::multimap<uint64_t,std::shared_ptr<spike_model::CacheRequest>>> in_flight_requests_per_l1;
        uint16_t available_mshrs;
        std::vector<uint64_t> mshr_stalls_per_core; //(num_cores);

        float avg_mem_access_time_l1_miss=0;
        float avg_time_to_reach_l2=0;
        uint64_t num_l2_accesses=1; //Initialized to one to calcullate a rolling average

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
        void submitToSparta(std::shared_ptr<spike_model::Event> r);

        /*!
         * \brief Submit a cache request to Sparta.
         * \param r The request to submit
         * \note This is equivalente to submitToSparta(std::shared_ptr<spike_model::Event> r), but it updates the number of in flight L1 misses
         */
        void submitToSparta(std::shared_ptr<spike_model::CacheRequest> r);

        /*
         * \brief Resume simulation on a core that is stalled
         * \param core The id of the core that will resume simulation
         */
        void resumeCore(uint64_t core);

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
