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

#ifndef __CORE_EVENT_HH__
#define __CORE_EVENT_HH__

#include <iostream>
#include "Event.hpp"

namespace coyote
{
    class CoreEvent : public coyote::Event
    {
        /**
         * \class coyote::CoreEvent
         *
         * \brief CoreEvent signals that an event related to a core has ocurred in Spike
         *
         */
        
        public:
            //CoreEvent(){}
            CoreEvent() = delete;
            CoreEvent& operator=(CoreEvent const&) = delete;

            /*!
             * \brief Constructor for CoreEvent
             * \param pc The program counter of the instruction that finished the simulation
             */
            CoreEvent(uint64_t pc): Event(), pc(pc){}

            /*!
             * \brief Constructor for CoreEvent
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The finishing core
             */
            CoreEvent(uint64_t pc, uint64_t time, uint16_t c): Event(time), pc(pc), coreId(c){}
            
            /*!
             * \brief Set the source tile of the event
             * \param source The source tile
             */
            void setSourceTile(uint16_t source)
            {
                source_tile=source;
            }

            /*!
             * \brief Get the source tile for the event
             * \return The source tile
             */
            uint16_t getSourceTile(){return source_tile;}

            /*!
             * \brief Get the eventing core
             * \return The core id
             */
            uint64_t getCoreId() const {return coreId;}

            /*!
             * \brief Set the eventing core
             * \param c The id of the core
             */
            void setCoreId(uint16_t c) {coreId=c;}

            /*!
             * \brief Get: the program counter for the eventing instruction
             * \return The PC
             */
            uint64_t getPC(){return pc;}

            
        private:
            uint64_t pc;
            uint16_t coreId;
            uint16_t source_tile;
    };
}
#endif
