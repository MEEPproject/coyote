#ifndef __FIFO_MEMORY_ACCESS_SCHEDULER_HH__
#define __FIFO_MEMORY_ACCESS_SCHEDULER_HH__

#include <queue>
#include <vector>
#include "CacheRequest.hpp"
#include "BankCommand.hpp"
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
            * \param b The banks handled by the scheduler
            * \param num_banks The number of banks
            * \param write_allocate True if writes need to allocate in cache
            */
            FifoRrMemoryAccessScheduler(std::shared_ptr<std::vector<MemoryBank *>> banks, uint64_t num_banks, bool write_allocate);

            /*!
            * \brief Add a request to the scheduler
            * \param req The request
            * \param bank The bank that the request targets
            */
            void putRequest(std::shared_ptr<CacheRequest> req, uint64_t bank) override;
            
            /*!
            * \brief Get the next bank for which a request should be handled. The bank is poped from the list.
            * \return The next bank in round-robin (out of the banks with requests)
            */
            uint64_t getNextBank() override;
            
            /*!
            * \brief Check there is any bank with pending requests that is idle
            * \return True if a request for a bank can be handled
            */
            virtual bool hasBanksToSchedule() override;
             
            /*!
            * \brief Notify the completion of a command to the scheduler
            * \return The request that has been serviced by this command. Null if no request was serviced
            */
            std::shared_ptr<CacheRequest> notifyCommandCompletion(std::shared_ptr<BankCommand> c) override;

            /*!
            * \brief Get the current queue occupancy
            * \return The number of requests that are currently enqueued in the scheduler. If the scheduler uses several queues. This is the aggregate value.
            */
            uint64_t getQueueOccupancy() override;
       
       protected: 
            /*!
            * \brief Get a command for a particular bank
            * \param bank The bank for which the request is desired
            * \return A command
            */
            virtual std::shared_ptr<CacheRequest> getRequest(uint64_t bank) override;
            
            /*!
            * \brief Notify that a request has been completed
            * \param req The request that was completed
            */
            virtual void notifyRequestCompletion(std::shared_ptr<CacheRequest> req) override;

        private:
            std::vector<std::queue<std::shared_ptr<CacheRequest>>> request_queues;
            std::queue<uint64_t> banks_to_schedule; //A bank id will be in this queue if the associated bank queue has elements and a request to the buffer is not currently in process
    };
}
#endif
