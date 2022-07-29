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

#ifndef __REGISTER_EVENT_HH__
#define __REGISTER_EVENT_HH__

#include <iostream>
#include "CoreEvent.hpp"

namespace coyote
{
    class RegisterEvent : public coyote::CoreEvent
    {
        /**
         * \class coyote::RegisterEvent
         *
         * \brief RegisterEvent signals that an event related to a register has ocurred in Spike
         *
         */
        
        public:
            enum class RegType
            {
                INTEGER,
                FLOAT,
                VECTOR,
                DONT_CARE,
            };

            //RegisterEvent(){}
            RegisterEvent() = delete;
            RegisterEvent(RegisterEvent const&) = delete;
            RegisterEvent& operator=(RegisterEvent const&) = delete;


            /*!
             * \brief Constructor for RegisterEvent
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The finishing core
             * \param reg The id od the register involved in the event
             * \param regType The type of the register
             * \param The destination register for the core event
             */
            RegisterEvent(uint64_t pc, uint64_t time, uint16_t c, uint8_t reg, coyote::RegisterEvent::RegType regType): CoreEvent(pc, time, c), regId(reg), regType(regType){}            

            /*!
             * \brief Set the event as serviced
             */
            void setServiced(){serviced=true;}
            
            /*!
             * \brief Returns if the event as serviced
             */
            bool isServiced(){return serviced;}
            
            /*!
             * \brief Set the id and type of the destination register for the event
             * \param r The id of the register
             * \param t The type of the register
             */
            void setDestinationReg(size_t r, RegType t) {regId=r; regType=t;}

            /*!
             * \brief Get the destination register id for the event
             * \return The id of the register
             */
            size_t getDestinationRegId() const { return regId;}

            /*!
             * \brief Get the type of the destination register for the event
             * \return The type of the register
             */
            RegType getDestinationRegType() const {return regType;}
            
        private:
            size_t regId;
            RegType regType;
            bool serviced=false;
    };
}
#endif
