#include "SpikeConnector.hpp"
#include <sstream>
#include <string.h>
#include <thread>   

#include "sparta/utils/SpartaAssert.hpp"

namespace spike_model
{
    SpikeConnector::SpikeConnector()
    {
        SpikeConnector(0, 0);
        dummy_connector=true;
    }

    SpikeConnector::SpikeConnector(uint32_t num_cores, unsigned int queue_size)
    {
        core_queues.resize(num_cores);
   
        for(SpikeInstructionStream& q: core_queues)
        {
            q.setSize(queue_size);
        }
 
        max_queue_size=queue_size;
        dummy_connector=false;
        
        mutex=std::make_shared<std::mutex>();
        condition=std::make_shared<std::condition_variable>();
    } 

    bool SpikeConnector::pushInstruction(std::shared_ptr<BaseInstruction> inst, unsigned int core)
    {
        
        bool res=core_queues[core].push(inst);

        //printf("Pushed to sparta. Size now is %d\n", core_queues[core].getWriteQueueSize());
        return res;
    }

    std::shared_ptr<BaseInstruction> SpikeConnector::getInstruction(int core)
    {
        bool swapped_queues=false;
        std::shared_ptr<BaseInstruction> insn=core_queues[core].pop(swapped_queues);

        if(SPARTA_EXPECT_FALSE(swapped_queues))
        {
            std::unique_lock<std::mutex> lock(*mutex);
            spike_needed=true;
                
            condition->notify_one();

                /*while(sparta_waiting)
                {
                    condition->wait(lock); //Wait for spike to do something, so Sparta is not faster and enters a deadlock
                }*/
        }

        //std::cout << "Popped from spike\n";
        return insn;
    }

            
    bool SpikeConnector::canWrite(unsigned int core, unsigned int steps)
    {
        if(SPARTA_EXPECT_FALSE(isDummy()))
        {
            return true;
        }
        else
        {
            if(SPARTA_EXPECT_FALSE(core_queues[core].isFinished()))
            {
                return false;
            }
            else
            {
                //printf("Can execute %d with current size %d? --> %d\n", steps, core_queues[core].getWriteQueueSize(), core_queues[core].getWriteQueueSize()+steps<max_queue_size);
                return core_queues[core].getWriteQueueSize()+steps<max_queue_size;
            }
        }
    }

    bool SpikeConnector::isDummy()
    {
        return dummy_connector;
    }

    void SpikeConnector::notifyCompletion(unsigned int core)
    {
        printf("Notifying completion\n");
        core_queues[core].setFinished();
    }

    
    void SpikeConnector::stopSpike(unsigned int with_step)
    {
        //printf("Trying to sleep\n");
        //CHECK IN A THREAD SAFE MANNER BEFORE GOING TO SLEEP
        std::unique_lock<std::mutex> lock(*mutex);


        //Might have a memory corruption in InstructionStreams that gets triggered by using this loop instead of the next
        /*for(SpikeInstructionStream s: core_queues)
        {
           s.notifySwap();
        }*/

        for(unsigned int i=0;i<core_queues.size();i++)
        {
           core_queues[i].notifySwap(); //Enable swap for everyone to avoid deadlock
        }

        //printf("SPIKE GOING TO SLEEP\n");
        while(!spike_needed)
        {   
            condition->wait(lock);
        }
        spike_needed=false;
        //printf("SPIKE WAKING UP\n");
    }
    
    void SpikeConnector::notifySwap(int core)
    {
        core_queues[core].notifySwap();
    }
            
    void SpikeConnector::setMaxQueueSize(unsigned int size)
    {
        max_queue_size=size;

        for(SpikeInstructionStream s: core_queues)
        {
           s.setSize(size); 
        }
    }
}
