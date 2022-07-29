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

#ifndef __VECTOR_WAITING_HH__
#define __VeCTOR_WAITING_HH__

#include "Sync.hpp"
#include "EventVisitor.hpp"
#include <iostream>

namespace coyote
{
    class VectorWaitingForScalarStore : public Sync, public std::enable_shared_from_this<VectorWaitingForScalarStore>
    {
        /**
         * \class coyote::VectorWaitingForScalarStore
         *
         * \brief VectorWaitingForScalarStore signals that the Spike simulation has been completed.
         *
         */
        
        public:

            //VectorWaitingForScalarStore(){}
            VectorWaitingForScalarStore() = delete;
            VectorWaitingForScalarStore(VectorWaitingForScalarStore const&) = delete;
            VectorWaitingForScalarStore& operator=(VectorWaitingForScalarStore const&) = delete;

            /*!
             * \brief Constructor for VectorWaitingForScalarStore
             * \param pc The program counter of the instruction that finished the simulation
             */
            VectorWaitingForScalarStore(uint64_t pc): Sync(pc){}

            /*!
             * \brief Constructor for VectorWaitingForScalarStore
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The finishing core
             */
            VectorWaitingForScalarStore(uint64_t pc, uint64_t time, uint16_t c): Sync(pc, time, c) {}

            
            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override
            {
                v->handle(shared_from_this());
            }

    };
}
#endif
