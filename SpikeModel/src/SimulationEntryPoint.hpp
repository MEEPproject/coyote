#ifndef __SIMULATION_ENTRY_POINT_HH__
#define __SIMULATION_ENTRY_POINT_HH__

class TraceDrivenSimulationOrchestrator; //Forward declaration

namespace spike_model
{
    class SimulationEntryPoint
    { 
        friend class FullSystemSimulationEventManager;
        friend class ::TraceDrivenSimulationOrchestrator;

        /**
         * \class spike_model::SimulationEntryPoint
         *
         * \brief SimulationEntryPoint objects can be input events as the starting point for simulation
         *
         */

        protected:
            /*!
             * \brief Submit an event.
             * \param ev The event to submit.
             * \note This method bypasses ports and timing and inputs the event straightaway. Thus, it should be used with care. 
             * For this reason, it is protected and to be used externally through friending. An example of adequate uses of this method can
             * be found in classes spike_model::FullSystemSimulationEventManager and spike_model::TraceDrivenSimulationOrchestrator.
             */ 
            virtual void putEvent(const std::shared_ptr<Event> &)=0;
    };
}
#endif
