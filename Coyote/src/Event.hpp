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

#ifndef __EVENT_HH__
#define __EVENT_HH__

#include "EventVisitor.hpp"

namespace coyote
{
    class EventVisitor;
    class Event
    {
        /**
         * \class coyote::Event
         *
         * \brief Event contains all the information regarding an event to be communicated from Spike to the rest of Coyote.
         *
         * Instances of Event (ant its children) are the main data structure that is communicated between Spike and the Sparta model
         * and between Sparta Units.
         *
         */
        
        public:

            Event& operator=(Event const&) = delete;

            /*!
             * \brief Constructor for Event
             */
            Event(): timestamp(0){}

            /*!
             * \brief Constructor for Event
             * \param time The timestamp for the request
             */
            Event(uint64_t time): timestamp(time){}

            /*!
             * \brief Get the timestamp of the event
             * \return The timestamp
             */
            uint64_t getTimestamp() const {return timestamp;}

            /*!
             * \brief Set the timestamp of the request
             * \param t The timestamp
             */
            void setTimestamp(uint64_t t) {timestamp=t;}
            

            /*
             * \brief Equality operator for instances of Request
             * \param m The request to compare with
             * \return true if both requests are for the same address
             */

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            virtual void handle(EventVisitor * v)=0;
            
            /*!
             * \brief Set the timestamp when the request reached the cache bank.
             * \param t The timestamp when the request reached the cache bank.
             */
            void setTimestampReachCacheBank(uint64_t t) {timestamp_reach_cache_bank=t;}

            /*!
             * \brief Get the timestamp when the request reached the cache bank.
             * \return The timestamp when the request reached the cache bank.
             */
            uint64_t getTimestampReachCacheBank() {return timestamp_reach_cache_bank;}
        
            /*!
             * \brief Set the timestamp when the request reached the memory controller.
             * \param t The timestamp when the request reached the memory controller.
             */
            void setTimestampReachMC(uint64_t t) {timestamp_reach_mc=t;}
            
            /*!
             * \brief Get the timestamp when the request reached the memory controller.
             * \return The timestamp when the request reached the memory controller.
             */
            uint64_t getTimestampReachMC() {return timestamp_reach_mc;}
            
            /*!
             * \brief Set the timestamp when the request reached the memory controller.
             * \param t The timestamp when the request reached the memory controller.
             */
            void setTimestampMCIssue(uint64_t t) {timestamp_mc_issue=t;}
            
            /*!
             * \brief Get the timestamp when the request reached the memory controller.
             * \return The timestamp when the request reached the memory controller.
             */
            uint64_t getTimestampMCIssue() {return timestamp_mc_issue;}

            /*!
             * \brief Set the timestamp when the request reached the tile arbiter.
             * \param t The timestamp when the request reached the tile arbiter.
             */
            void setTimestampReachArbiter(uint64_t t) {timestamp_reach_arbiter=t;}
            
            /*!
             * \brief Get the timestamp when the request reached the tile arbiter.
             * \return The timestamp when the request reached the tile arbiter.
             */
            uint64_t getTimestampReachArbiter() {return timestamp_reach_arbiter;}
            

        private:
            uint64_t timestamp;
            
            uint64_t timestamp_reach_cache_bank=0;
            uint64_t timestamp_reach_mc=0;
            uint64_t timestamp_mc_issue=0;
            uint64_t timestamp_reach_arbiter=0;
    };
    
/*    inline std::ostream & operator<<(std::ostream & Str, Request const & req)
    {
        Str << "0x" << std::hex << req.getAddress() << " @ " << req.getTimestamp();
        return Str;
    }*/
}
#endif
