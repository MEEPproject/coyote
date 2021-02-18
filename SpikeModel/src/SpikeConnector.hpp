#ifndef __SPIKE_CONNECTOR_H__
#define __SPIKE_CONNECTOR_H__

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "SpikeInstructionStream.hpp"

#include "Instructions.hpp"

namespace spike_model
{
    /**
     * @file   SpikeConnector.h
     * @brief This is the interface between Sparta and Spike.
     *
     * SpikeConnector
     * 1. Launches a thread that runs Spike.
     * 2. Handles communication between both simulators
     */
    class SpikeConnector
    {
    
        private:
            std::vector<SpikeInstructionStream> core_queues;

            unsigned int max_queue_size;
            
            std::shared_ptr<std::mutex> mutex;

            std::shared_ptr<std::condition_variable> condition;

            //std::thread spike_thread;

            bool dummy_connector;
    
            bool spike_needed=true;

        public:

            SpikeConnector();

            SpikeConnector(uint32_t num_cores, unsigned int queue_size=1000);
            
            ~SpikeConnector(){};

            bool pushInstruction(std::shared_ptr<BaseInstruction> inst, unsigned int core);

            std::shared_ptr<BaseInstruction> getInstruction(int core);
            
            unsigned int numCores(){return core_queues.size();};

            void setMaxQueueSize(unsigned int size);

            bool isDummy();
            
            bool canWrite(unsigned int core, unsigned int steps);

            void stopSpike(unsigned int with_step);

            // Copy assignment operator
            // The thread does not get copied, as Spike does not need access to it.
            /*SpikeConnector  &operator=(const SpikeConnector & rhs)
            {
                core_queues=rhs.core_queues;
                max_queue_size=rhs.max_queue_size;
                dummy_connector=rhs.dummy_connector;

                return *this;
            }*/

            void notifyCompletion(unsigned int core);
    
            void notifySwap(int core);
    }; 
}
#endif
