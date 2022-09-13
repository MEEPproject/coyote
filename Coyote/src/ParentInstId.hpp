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

#ifndef __PARENT_INST_ID_HH__
#define __PARENT_INST_ID_HH__

namespace coyote
{
    class ParentInstId
    {
        /**
         * \class coyote::ParentInstId
         *
         * \brief ParentInstId contains all the information regarding an event to be communicated from Spike to the rest of Coyote.
         *
         * Instances of ParentInstId (ant its children) are the main data structure that is communicated between Spike and the Sparta model
         * and between Sparta Units.
         *
         */
        
        public:

            ParentInstId& operator=(ParentInstId const&) = delete;

            /*!
             * \brief Constructor for ParentInstId
             */
            ParentInstId(): id(0){}
 
            /*!
             * \brief Get the ID of the instruction.
             * We use that in the Memory Tile to map original instructions to generated ones.
             */
            uint32_t getID() {return id;}
            
            /*!
             * \brief Set the ID of the instruction.
             * \param The ID to be set.
             */
            void setID(uint32_t id) {this->id = id;}

        private:
            uint32_t id;    // used to identify the parent instruction

    };
    
}
#endif
