#ifndef __MEMORY_ACCESS_SCHEDULER_HH__
#define __MEMORY_ACCESS_SCHEDULER_HH__

#include "sparta/simulation/Unit.hpp"

#include <memory>
#include "Request.hpp"
#include "MemoryBank.hpp"

namespace spike_model
{
    class MemoryBank; //Forward declaration

    class MemoryAccessSchedulerIF
    {
        /*!
         * \class spike_model::MemoryAccessSchedulerIF
         * \brief Abstract class representing the minimum operation of a memory access scheduler for Request instances.
         */
        public: 
            
            /*!
            * \brief Constructor for FifoRrMemoryAccessSchedulerAccessTypePriority
            * \param b The number banks handled by the scheduler
            * \param num_banks The number of banks
            * \param write_allocate True if writes need to allocate in cache
            */
            MemoryAccessSchedulerIF(std::shared_ptr<std::vector<MemoryBank *>> b, uint64_t num_banks, bool write_allocate);

            /*!
            * \brief Add a request to the scheduler
            * \param req The request
            * \param bank The bank that the request targets
            */
            virtual void putRequest(std::shared_ptr<CacheRequest> req, uint64_t bank)=0;
            
            /*!
            * \brief Get a command for a particular bank
            * \param bank The bank for which the request is desired
            * \return A command
            */
            std::shared_ptr<BankCommand> getCommand(uint64_t bank);
             
            /*!
            * \brief Get the next bank for which a request should be handled
            * \return The next bank
            */
            virtual uint64_t getNextBank()=0;

            /*!
            * \brief Check there is any bank with pending requests that is idle
            * \return True if a request for a bank can be handled
            */
            virtual bool hasBanksToSchedule()=0;

            /*!
            * \brief Notify the completion of a command to the scheduler
            * \return The request that has been serviced by this command. Null if no request was serviced
            */
            virtual std::shared_ptr<CacheRequest> notifyCommandCompletion(std::shared_ptr<BankCommand> c);

            /*!
            * \brief Get the current queue occupancy
            * \return The number of requests that are currently enqueued in the scheduler. If the scheduler uses several queues. This is the aggregate value.
            */
            virtual uint64_t getQueueOccupancy()=0;

            /*!
            * \brief Check if the timing of the earlier commands allows the request to be handled now
            * \return True if the bank is ready for the request 
            */
            bool isBankReady(std::shared_ptr<CacheRequest> req);

       protected: 
            /*!
            * \brief Get a command for a particular bank
            * \param bank The bank for which the request is desired
            * \return A command
            */
            virtual std::shared_ptr<CacheRequest> getRequest(uint64_t bank)=0;
            
            /*!
            * \brief Notify that a request has been completed
            * \param req The request that was completed
            */
            virtual void notifyRequestCompletion(std::shared_ptr<CacheRequest> req)=0;

        private:
            std::shared_ptr<std::vector<MemoryBank *>> banks;
            bool write_allocate;
            std::vector<BankCommand::CommandType> last_completed_command_per_bank;

            /*!
             * \brief Create a command to access the memory using the type of the associated request (READ or WRITE)
             * \param req The request
             * \param bank The bank to access
             * \return A command of the correct type to access the specified bank
             */
            std::shared_ptr<BankCommand> getAccessCommand_(std::shared_ptr<CacheRequest> req, uint64_t bank);

            /*!
             * \brief Create a command to read the line after writing it (if write-allocate is enabled)
             * \param req The request
             * \param bank The bank to access
             * \return A command for the write allocate
             */
            std::shared_ptr<BankCommand> getAllocateCommand_(std::shared_ptr<CacheRequest> req, uint64_t bank);
    };
}
#endif
