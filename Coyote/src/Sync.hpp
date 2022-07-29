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

#ifndef __SYNC_HH__
#define __SYNC_HH__

#include <iostream>
#include "CoreEvent.hpp"

namespace coyote
{
    class Sync : public coyote::CoreEvent
    {
        /**
         * \class coyote::Sync
         *
         * \brief Sync signals that the Spike simulation has been completed.
         *
         */
        
        public:

            //Sync(){}
            Sync() = delete;
            Sync(Sync const&) = delete;
            Sync& operator=(Sync const&) = delete;

            /*!
             * \brief Constructor for Sync
             * \param pc The program counter of the instruction that finished the simulation
             */
            Sync(uint64_t pc): CoreEvent(pc){}

            /*!
             * \brief Constructor for Sync
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The finishing core
             */
            Sync(uint64_t pc, uint64_t time, uint16_t c): CoreEvent(pc, time, c) {}
    };
}
#endif
