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


#ifndef __BANK_COMMAND_HH__
#define __BANK_COMMAND_HH__

#include <Request.hpp>

namespace coyote
{
    class BankCommand
    {
        /*!
         * \class coyote::BankCommand
         * \brief Models a MemoryBank command.
         *
         * Only a limited subset of commands is currently supported, specified in enum CommandType
         */
        public:
            enum class CommandType
            {
                ACTIVATE,
                PRECHARGE,
                READ,
                WRITE,
                NUM_COMMANDS
            };

            BankCommand() = delete;
            BankCommand(BankCommand const&) = delete;
            BankCommand& operator=(BankCommand const&) = delete;
            
            /*!
             * \brief Constructor for BankCommand
             * \param t The type of the command
             * \param b The bank that will be accessed
             * \param v The value associated to the command. E.g. The row to open or the column to read
             * \param r The request associated to this command
             * \note If commands get sufficiently complex an inheritance-based approach would make sense
             */
            BankCommand(CommandType t, uint64_t b, uint64_t v, std::shared_ptr<CacheRequest> r)
            {
                type=t;
                destination_bank=b;
                value=v;
                request=r;
                high_priority=false;
            }

            /*!
             * \brief Get the type of the command
             * \return The type
             */
            CommandType getType()
            {
                return type;
            }

            /*!
             * \brief Get the value of the command, such as the column to be read or the row to open
             * \return The value
             */
            uint64_t getValue()
            {
                return value;
            }

            /*!
             * \brief Get the bank that the command will access
             * \return The bank of the command
             */
            uint64_t getDestinationBank()
            {
                return destination_bank;
            }

            /*!
             * \brief Get the request associated to this command
             * \return The associated request
             */
            std::shared_ptr<CacheRequest> getRequest()
            {
                return request;
            }

            /*!
             * \brief Marks this command as the one completing its associated request
             */
            void setCompletesRequest()
            {
                completes_request=true;
            }

            /*!
             * \brief Checks if this command completes its associated request
             */
            bool getCompletesRequest()
            {
                return completes_request;
            }

            bool isHighPriority()
            {
                return high_priority;
            }
            
            void setHighPriority()
            {
                high_priority=true;
            }

        private:
            CommandType type;
            uint64_t destination_bank;
            uint64_t value;

            bool high_priority;

            bool completes_request=false;

            std::shared_ptr<CacheRequest> request;
    };
}
#endif
