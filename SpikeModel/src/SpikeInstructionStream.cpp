#include "SpikeInstructionStream.hpp"

#include "StateInstruction.hpp"

#include "sparta/utils/SpartaAssert.hpp"

namespace spike_model
{
    SpikeInstructionStream::SpikeInstructionStream() : write_queue(), read_queue()
    {
        mutex=std::make_shared<std::mutex>();
        condition=std::make_shared<std::condition_variable>();

    }
    
    SpikeInstructionStream::SpikeInstructionStream(unsigned int size) : write_queue(), read_queue()
    {
        SpikeInstructionStream();
        max_size=size;
        finished=false;
    }

    bool SpikeInstructionStream::push(std::shared_ptr<BaseInstruction> inst)
    {
        bool result=false;
        if(SPARTA_EXPECT_TRUE(write_queue.size()<max_size))
        {
            write_queue.push(inst);
            result=true;
        }
        
        if(write_queue.size()==max_size)
        {
            notifySwap();
        }

        return result;
    }

    std::shared_ptr<BaseInstruction> SpikeInstructionStream::pop(bool& swapped_queues)
    {
        std::shared_ptr<BaseInstruction> res;
        swapped_queues=false;
        if(read_queue.size()==0)
        {
            bool success=false;
            //This is a busy wait, but should not be problematic considering the performance diference between Sparta and Spike
            //If found problematic, it should be replaced with a conditional wait
            while(!success)
            {
                std::unique_lock<std::mutex> lock(*mutex);
                if(can_swap)//likely - Sparta and Spike use two different defines likely/unlikely and SPARTA_EXPECT_FALSE/SPARTA_EXPECT_TRUE
                {
                    swap(write_queue, read_queue);
                    success=true;
                    swapped_queues=true;
                    can_swap=false;
                    //printf("SWAPPING\n");
                }
                else if(finished)
                {
                    std::shared_ptr<StateInstruction> end_insn (new StateInstruction(0, StateInstruction::Type::FINISH));
                    read_queue.push(end_insn);
                    success=true;
                    std::cout << "End instruction self enqueued\n";

                    //A core, upon receiving, must do something like -> scheduleEvent(stop_event_.get(), num_ticks - 1, 0, exacting_run); Maybe when every core has received
                }
                else
                {
                    while(!can_swap && !finished)
                    {
                        //printf("Going to bed\n");
                        condition->wait(lock);
                        //printf("Waking up\n");
                    }
                }
            }
        }
        res=read_queue.front();
        read_queue.pop(); 
        //std::cout << "Returning instruction " << *res << "\n";
        return res;
    }

    void SpikeInstructionStream::setSize(unsigned int size)
    {
        max_size=size;
    }

    unsigned int SpikeInstructionStream::getWriteQueueSize()
    {
        unsigned int res;
        mutex->lock();
        res=write_queue.size();
        mutex->unlock();
        return res;
    }
    
    void SpikeInstructionStream::setFinished()
    {
        printf("Setting finished\n");
        mutex->lock();
        finished=true;
        mutex->unlock();

        notifySwap(); //Whatever is left must be completed
    }

    void SpikeInstructionStream::notifySwap()
    { 
        std::unique_lock<std::mutex> lock(*mutex);
        
        if(write_queue.size()>0)
        {
            can_swap=true;
        }
        condition->notify_one();

    }
}
