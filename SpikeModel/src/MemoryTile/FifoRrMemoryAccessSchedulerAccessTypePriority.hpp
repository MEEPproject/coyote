#ifndef __FIFO_MEMORY_ACCESS_SCHEDULER_ACCESS_TYPE_PRIORITY_HH__
#define __FIFO_MEMORY_ACCESS_SCHEDULER_ACCESS_TYPE_PRIORITY_HH__

#include <queue>
#include <vector>
#include "CacheRequest.hpp"
#include "MemoryAccessSchedulerIF.hpp"

namespace spike_model
{
    class FifoRrMemoryAccessSchedulerAccessTypePriority : public MemoryAccessSchedulerIF
    {
        /*!
         * \class spike_model::FifoRrMemoryAccessSchedulerAccessTypePriority
         * \brief A simple memory access scheduler that separates requests in different for fetches, loads and stores. The scheduler follows the following policy:
         * 1. The next bank to schedule is chosen in round-robin
         * 2. The next request to schedule is chosen in FIFO, evaluating the queues in the following order: feteches, loads, stores
         */
        public:
 
            /*!
            * \brief Constructor for FifoRrMemoryAccessSchedulerAccessTypePriority
            * \param num_banks The number of banks handled by the scheduler
            */
            FifoRrMemoryAccessSchedulerAccessTypePriority(uint64_t num_banks);

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
            
        private:
            std::vector<std::queue<std::shared_ptr<CacheRequest>>> fetch_queues;
            std::vector<std::queue<std::shared_ptr<CacheRequest>>> load_queues;
            std::vector<std::queue<std::shared_ptr<CacheRequest>>> store_queues;

            std::queue<uint64_t> banks_to_schedule; //A bank id will be in this queue if the associated bank queue has elements and a request to the buffer is not currently in process
    };
}
#endif
