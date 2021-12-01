#ifndef __MEMORY_ACCESS_SCHEDULER_HH__
#define __MEMORY_ACCESS_SCHEDULER_HH__

#include "sparta/simulation/Unit.hpp"

#include <memory>
#include "Request.hpp"

namespace spike_model
{
    class MemoryAccessSchedulerIF
    {
        /*!
         * \class spike_model::MemoryAccessSchedulerIF
         * \brief Abstract class representing the minimum operation of a memory access scheduler for Request instances.
         */
        public: 

            /*!
            * \brief Add a request to the scheduler
            * \param req The request
            * \param bank The bank that the request targets
            */
            virtual void putRequest(std::shared_ptr<CacheRequest> req, uint64_t bank)=0;
            
            /*!
            * \brief Get a request for a particular bank
            * \param bank The bank for which the request is desired
            * \return A request
            */
            virtual std::shared_ptr<CacheRequest> getRequest(uint64_t bank)=0;
            
            /*!
            * \brief Get the next bank for which a request should be handled
            * \return The next bank
            */
            virtual uint64_t getNextBank()=0;

            /*!
            * \brief Check there is any bank with pending requests that is idle
            * \return True if a request for a bank can be handled
            */
            virtual bool hasIdleBanks()=0;

            /*!
            * \brief Notify that a request has been completed
            * \param bank The bank which completed the request
            */
            virtual void notifyRequestCompletion(std::shared_ptr<CacheRequest> req)=0;

            /*!
            * \brief Get the current queue occupancy
            * \return The number of requests that are currently enqueued in the scheduler. If the scheduler uses several queues. This is the aggregate value.
            */
            virtual uint64_t getQueueOccupancy()=0;

    };
}
#endif
