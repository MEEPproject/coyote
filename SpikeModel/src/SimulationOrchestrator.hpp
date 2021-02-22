
#ifndef __SIMULATION_ORCHESTRATOR_HH__
#define __SIMULATION_ORCHESTRATOR_HH__

#include <memory>
#include "spike_wrapper.h"
#include "RequestManagerIF.hpp"
#include "SpikeModel.hpp"
#include "LogCapable.hpp"

class SimulationOrchestrator : public spike_model::LogCapable
{
    
    /**
     * \class SimulationOrchestrator
     *
     * \brief SimulationOrchestrator is the glue that puts together  Spike and Sparta in Coyote.
     *
     * Its purpose is to handle the execution of instructions on Spike, the forwarding of L2 Requests
     * to Sparta and the notifications to Sparta when a request has been serviced.
     *
     */

    public:

        /*!
         * \brief Constructor for the SimulationOrchestrator
         * \param spike An instance of a wrapped Spike simulation
         * \param spike_model The model for the architecture that will be simulated
         * \param request_manager The request manager to communicate requests to the modelled architecture
         * \param num_cores The number of simukated cores
         * \param trace Whether tracing is enabled or not
         */
        SimulationOrchestrator(std::shared_ptr<spike_model::SpikeWrapper>& spike, std::shared_ptr<SpikeModel>& spike_model, std::shared_ptr<spike_model::RequestManagerIF>& request_manager, uint32_t num_cores, uint32_t num_threads_per_core, uint32_t thread_switch_latency, bool trace);

        /*!
         * \brief Triggers the simulation
         */
        void run();

    private:
        std::shared_ptr<spike_model::SpikeWrapper> spike;
        std::shared_ptr<SpikeModel> spike_model;
        std::shared_ptr<spike_model::RequestManagerIF> request_manager;
        uint32_t num_cores;
        uint32_t num_threads_per_core;
        uint32_t thread_switch_latency;


        std::vector<uint16_t> active_cores;
        std::vector<uint16_t> stalled_cores;
        std::vector<uint16_t> runnable_cores;
        std::vector<uint64_t> runnable_after;
        std::vector<uint16_t> cur_cycle_suspended_threads;
        std::vector<bool> threads_in_barrier;

        std::vector<std::list<std::shared_ptr<spike_model::Request>>> pending_misses_per_core; //(num_cores);
        std::vector<std::list<std::shared_ptr<spike_model::Request>>> pending_writebacks_per_core; //(num_cores);
        std::vector<uint64_t> simulated_instructions_per_core; //(num_cores);

        uint64_t thread_barrier_cnt;
        uint64_t current_cycle;
        uint64_t next_event_tick;
        uint64_t timer;        
        bool spike_finished;

        bool trace;

        /*!
         * \brief Simulate an instruction in each of the active cores
         */
        void simulateInstInActiveCores();
        
        /*!
         * \brief Handle the events for cycles earlier or equal to the current cycle
         */
        void handleEvents();

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
};
#endif
