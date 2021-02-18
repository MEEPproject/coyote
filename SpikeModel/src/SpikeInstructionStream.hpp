#ifndef __SPIKE_INSTRUCTION_STREAM_H__
#define __SPIKE_INSTRUCTION_STREAM_H__

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "BaseInstruction.hpp"


namespace spike_model
{
    class BaseInstruction;
    class SpikeInstructionStream
    {
        private:
            std::queue<std::shared_ptr<BaseInstruction>> write_queue;
            std::queue<std::shared_ptr<BaseInstruction>> read_queue;
            std::shared_ptr<std::mutex> mutex;
            std::shared_ptr<std::condition_variable> condition;
    
            unsigned int max_size;

            bool finished=false;           
       
            bool can_swap=false;
 
        public:
            SpikeInstructionStream();    
        
            SpikeInstructionStream(unsigned int size);

            bool push(std::shared_ptr<BaseInstruction> inst);

            std::shared_ptr<BaseInstruction> pop(bool& swapped_queues);

            unsigned int getWriteQueueSize();
            
            void setSize(unsigned int size);

            void setFinished();

            bool isFinished(){return finished;};
            
            void notifySwap();
    };
}
#endif
