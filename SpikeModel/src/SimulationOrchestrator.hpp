#ifndef __SIMULATION_ORCHESTRATOR_HH__
#define __SIMULATION_ORCHESTRATOR_HH__

class SimulationOrchestrator : public spike_model::LogCapable
{
    
    /**
     *
     * \brief SimulationOrchestrator is the glue that puts together  Spike and Sparta in Coyote.
     *
     * Its purpose is to handle the execution of instructions on Spike, the forwarding of L2 Requests
     * to Sparta and the notifications to Sparta when a request has been serviced.
     *
     */

    public:

        /*!
         * \brief Triggers the simulation
         */
        virtual void run()=0;
        
        /*!
         * \brief Show the statistics of the simulation
         */
        virtual void saveReports(){};
};
#endif
