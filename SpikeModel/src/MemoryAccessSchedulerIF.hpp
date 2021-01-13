#ifndef __MEMORY_ACCESS_SCHEDULER_HH__
#define __MEMORY_ACCESS_SCHEDULER_HH__

#include <memory>
#include "Request.hpp"

namespace spike_model
{
    class MemoryAccessSchedulerIF
    {
        public: 

            virtual void putRequest(std::shared_ptr<Request> req, uint64_t bank)=0;
            
            virtual std::shared_ptr<Request> getRequest(uint64_t bank)=0;
            virtual uint64_t getNextBank()=0;

            virtual bool hasIdleBanks()=0;

            virtual void notifyRequestCompletion(uint64_t bank)=0;
    };
}
#endif
