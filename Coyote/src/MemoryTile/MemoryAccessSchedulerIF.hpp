// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 

#ifndef __MEMORY_ACCESS_SCHEDULER_HH__
#define __MEMORY_ACCESS_SCHEDULER_HH__

#include "sparta/simulation/Unit.hpp"

#include <memory>
#include "Request.hpp"
#include "MemoryBank.hpp"

namespace coyote
{
    class MemoryBank; //Forward declaration

    class MemoryAccessSchedulerIF
    {
        /*!
         * \class coyote::MemoryAccessSchedulerIF
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
            * \return A command (or nullptr if no command is available)
            */
            virtual std::shared_ptr<BankCommand> getCommand(uint64_t bank);
             
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

            /*
             * \brief Notify that a command has been submitted to its bank
             * \param c The command that has been submitted
             */
            void notifyCommandSubmission(std::shared_ptr<BankCommand> c);

            /*!
            * \brief Get the current queue occupancy
            * \return The number of requests that are currently enqueued in the scheduler. If the scheduler uses several queues. This is the aggregate value.
            */
            virtual uint64_t getQueueOccupancy()=0;

        private:
            bool write_allocate;


            /*!
             * \brief Create a command to access the memory using the type of the associated request (READ or WRITE)
             * \param req The request
             * \param bank The bank to access
             * \return A command of the correct type to access the specified bank
             */
            std::shared_ptr<BankCommand> getAccessCommand_(std::shared_ptr<CacheRequest> req, uint64_t bank);
    
            /*
             * \brief Check if a command completes its associated request
             * \param c The command that has been submitted
             * \return true if the request has been completed
             */
            void checkRequestCompletion(std::shared_ptr<BankCommand> c);
       
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
            
            /*!
             * \brief Mark a bank as ready for scheduling if it has any pending requests
             * \param c The bank to reschedule
             */
            virtual void rescheduleBank(uint64_t bank)=0;
            
            std::vector<bool> pending_command; //pending_command[i]==true if there is a command for bank i that has been sent to the command scheduler that has not been submitted yet
            std::shared_ptr<std::vector<MemoryBank *>> banks;
     
    };
}
#endif
