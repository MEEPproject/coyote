#ifndef __FIFO_MEMORY_ACCESS_SCHEDULER_HH__
#define __FIFO_MEMORY_ACCESS_SCHEDULER_HH__

#include <queue>
#include <vector>
#include "CacheRequest.hpp"
#include "MemoryAccessSchedulerIF.hpp"

namespace spike_model
{
    class FifoRrMemoryAccessScheduler : public MemoryAccessSchedulerIF
    {
        /*!
         * \class spike_model::FifoRrMemoryAccessScheduler
         * \brief A simple memory access scheduler that follows the following policy.
         * 1. The next bank to schedule is chosen in round-robin
         * 2. The next request to schedule is chosen in FIFO
         */
        public:
 
            /*!
            * \brief Constructor for FifoRrMemoryAccessScheduler
            * \param num_banks The number of banks handled by the scheduler
            */
            FifoRrMemoryAccessScheduler(uint64_t num_banks);

            /*!
            * \brief Add a request to the scheduler
            * \param req The request
            * \param bank The bank that the request targets
            */
            void putRequest(std::shared_ptr<CacheRequest> req, uint64_t bank) override;
            
            /*!
            * \brief Get a request for a particular bank
            * \param bank The bank for which the request is desired
            * \return The first request for the bank in FIFO
            */
            std::shared_ptr<CacheRequest> getRequest(uint64_t bank) override;

            /*!
            * \brief Get the next bank for which a request should be handled. The bank is poped from the list.
            * \return The next bank in round-robin (out of the banks with requests)
            */
            uint64_t getNextBank() override;
            
            /*!
            * \brief Check there is any bank with pending requests that is idle
            * \return True if a request for a bank can be handled
            */
            virtual bool hasIdleBanks() override;
            
            /*!
            * \brief Notify that a request has been completed
            * \param req The request that has been completed
            */
            void notifyRequestCompletion(std::shared_ptr<CacheRequest> req) override;
            
            /*!
            * \brief Get the current queue occupancy
            * \return The number of requests that are currently enqueued in the scheduler. If the scheduler uses several queues. This is the aggregate value.
            */
            uint64_t getQueueOccupancy() override;

        private:
            std::vector<std::queue<std::shared_ptr<CacheRequest>>> request_queues;
            std::queue<uint64_t> banks_to_schedule; //A bank id will be in this queue if the associated bank queue has elements and a request to the buffer is not currently in process
    };
}
#endif
