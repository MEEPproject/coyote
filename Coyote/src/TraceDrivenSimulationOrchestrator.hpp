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


#ifndef __TRACE_DRIVEN_SIMULATION_ORCHESTRATOR_HH__
#define __TRACE_DRIVEN_SIMULATION_ORCHESTRATOR_HH__
    
#include <memory>
#include "spike_wrapper.h"
#include "FullSystemSimulationEventManager.hpp"
#include "Coyote.hpp"
#include "Event.hpp"
#include "EventVisitor.hpp"
#include "CacheRequest.hpp"
#include "ScratchpadRequest.hpp"
#include "MemoryTile/MCPUSetVVL.hpp"
#include "MemoryTile/MCPUInstruction.hpp"
#include <set>
#include <map>
#include <fstream>

#include "Finish.hpp"
#include "LogCapable.hpp"
#include "SimulationOrchestrator.hpp"
#include "NoC/NoC.hpp"

class TraceDrivenSimulationOrchestrator : public SimulationOrchestrator
{
    
    /**
     * \class TraceDrivenSimulationOrchestrator
     *
     * \brief TraceDrivenSimulationOrchestrator is the glue that puts together  Spike and Sparta in Coyote.
     *
     * Its purpose is to handle the execution of instructions on Spike, the forwarding of L2 Requests
     * to Sparta and the notifications to Sparta when a request has been serviced.
     *
     */

    public:

        /*!
         * \brief Constructor for the TraceDrivenSimulationOrchestrator
         * \param trace_path The path of the trace to be simulated
         * \param coyote The model for the architecture that will be simulated
         * \param entry_point The entry point for the event engine
         * \param trace Whether tracing is enabled or not
         * \param noc A pointer to the simulated NoC
         */
        TraceDrivenSimulationOrchestrator(std::string trace_path, std::shared_ptr<Coyote>& coyote, coyote::SimulationEntryPoint * entry_point, bool trace, coyote::NoC* noc);

        /*!
         * \brief Destructor for TraceDrivenSimulationOrchestrator
         */
        ~TraceDrivenSimulationOrchestrator();

        /*!
         * \brief Triggers the simulation
         */
        void run();


    private:
        std::shared_ptr<Coyote>& coyote;
        coyote::SimulationEntryPoint * entry_point;
        
        uint64_t current_cycle;
        bool trace;

        coyote::NoC* noc_;    //! Pointer to the NoC

        std::ifstream input_trace;
        
        /*!
         * \brief Parse a line from the trace and generate its associated event
         * \param line The line to parse
         * \return The event associated to the line
         */
        std::shared_ptr<coyote::Event> parse(std::string& line);


        /*!
         * \brief Produce a cache request object
         * \param address The addres for the request
         * \param t The type of request
         * \param pc The PC of the generating instruction
         * \param timestamp The timestamp of the request
         * \param core The id of the producing corei
         * \param size The size of accessed data
         * \return A cache request with the specified values
         */
        std::shared_ptr<coyote::CacheRequest> createCacheRequest(std::string& address, coyote::CacheRequest::AccessType t, std::string& pc, std::string& timestamp, std::string& core, std::string& size);
};
#endif
