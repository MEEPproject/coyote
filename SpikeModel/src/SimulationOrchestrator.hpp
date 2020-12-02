
#ifndef __SIMULATION_ORCHESTRATOR_HH__
#define __SIMULATION_ORCHESTRATOR_HH__

#include <memory>
#include "spike_wrapper.h"
#include "RequestManager.hpp"
#include "SpikeModel.hpp"
#include "LogCapable.hpp"

class SimulationOrchestrator : public spike_model::LogCapable
{
    public:

        /*!
         * \brief Constructor for the SimulationOrchestrator
         */
        SimulationOrchestrator(std::shared_ptr<spike_model::SpikeWrapper>& spike, std::shared_ptr<SpikeModel>& spike_model, std::shared_ptr<spike_model::RequestManager>& request_manager, uint32_t num_cores, bool trace);

        /*!
         * \brief Runs the simulation
         */
        void run();

    private:
        std::shared_ptr<spike_model::SpikeWrapper> spike;
        std::shared_ptr<SpikeModel> spike_model;
        std::shared_ptr<spike_model::RequestManager> request_manager;
        uint32_t num_cores;

        std::vector<uint16_t> active_cores;
        std::vector<uint16_t> stalled_cores;
        std::vector<std::list<std::shared_ptr<spike_model::L2Request>>> pending_misses_per_core; //(num_cores);
        std::vector<std::list<std::shared_ptr<spike_model::L2Request>>> pending_writebacks_per_core; //(num_cores);
        std::vector<uint64_t> simulated_instructions_per_core; //(num_cores);
    
        uint64_t current_cycle;
        uint64_t next_event_tick;
        uint64_t timer;        
        bool spike_finished;

        bool trace;

        /*!
         * \brief Simulates an instruction in each of the active cores
         */
        void simulateInstInActiveCores();
        
        /*!
         * \brief Handles the events for events earlier than the current cycle
         */
        void handleEvents();

        /*!
         * \brief Calcullates the difference between a cycle value and Sparta time.
         */
        uint64_t spartaDelay(uint64_t cycle);
};
#endif
