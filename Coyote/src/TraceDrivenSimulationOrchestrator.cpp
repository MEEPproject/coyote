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

#include <sstream>
#include <string>

#include "TraceDrivenSimulationOrchestrator.hpp"
#include "CacheRequest.hpp"


TraceDrivenSimulationOrchestrator::TraceDrivenSimulationOrchestrator(std::string trace_path, std::shared_ptr<Coyote>& coyote, coyote::SimulationEntryPoint * entry_point, bool trace, coyote::NoC* noc) :
    coyote(coyote),
    entry_point(entry_point),
    current_cycle(1),
    trace(trace),
    noc_(noc),
    input_trace(trace_path)
{
}


TraceDrivenSimulationOrchestrator::~TraceDrivenSimulationOrchestrator()
{
    //PRINTS THE SPARTA STATISTICS
    coyote->saveReports();
}


void TraceDrivenSimulationOrchestrator::run()
{
    /* Open file
     * Check error
     * While lines in trace
     *  Get line
     *  If time in line > time -> advance sparta
     *  submit event
     * Finish simulation
    */
    std::string line;
    std::getline(input_trace, line); // skip the first line, which contains headers
    while (std::getline(input_trace, line))
    {
        if(line!="")//Skip empty lines
        {
            std::shared_ptr<coyote::Event> event=parse(line);
            if(event->getTimestamp()>current_cycle)
            {
                uint64_t cycles=event->getTimestamp()-coyote->getScheduler()->getCurrentTick(); //runRaw not inclusive. All events prior to the one in the timestamp handled
                std::cout << "Advancing " << cycles << " cycles starting from t " << coyote->getScheduler()->getCurrentTick() << "\n";
                coyote->runRaw(cycles);
                std::cout << "Now the clock is " << coyote->getScheduler()->getCurrentTick() << "\n";
                current_cycle=event->getTimestamp();
            }
            entry_point->putEvent(event);
        }
    }

    std::cout << "After 1st loop\n";

 //   uint64_t next_event_tick=coyote->getScheduler()->nextEventTick();

    //Handle pending events
    coyote->run(sparta::Scheduler::INDEFINITE);
    /*while(next_event_tick!=sparta::Scheduler::INDEFINITE)
    {
        uint64_t cycles=next_event_tick-coyote->getScheduler()->getCurrentTick()+1; //runRaw not inclusive. All events handled included the one in next_event_tick
        std::cout << "Next is " <<  next_event_tick << ", current is " << coyote->getScheduler()->getCurrentTick() << ". Advancing " << cycles << " cycles starting from t " << coyote->getScheduler()->getCurrentTick() << "\n";
        coyote->runRaw(cycles);
        next_event_tick=coyote->getScheduler()->nextEventTick();
    }*/
    std::cout << "After 2nd loop\n";
    //coyote->saveReports();
}

std::shared_ptr<coyote::CacheRequest> TraceDrivenSimulationOrchestrator::createCacheRequest(std::string& address, coyote::CacheRequest::AccessType t, std::string& pc, std::string& timestamp, std::string& core, std::string& size)
{
    std::shared_ptr<coyote::CacheRequest> req=std::make_shared<coyote::CacheRequest>(std::stoull(address.c_str(), nullptr, 16), t, std::stoull(pc.c_str(), nullptr, 16), std::stoull(timestamp.c_str()), std::stoi(core.c_str()));
    req->setSize(std::stoi(size.c_str()));
    return req;
}

std::shared_ptr<coyote::Event> TraceDrivenSimulationOrchestrator::parse(std::string& line)
{
        std::string timestamp, core, pc, type, v1, v2;
        std::istringstream iss(line);
        getline(iss, timestamp,',');
        getline(iss, core,',');
        getline(iss, pc,',');
        getline(iss, type,',');
        getline(iss, v1,',');
        getline(iss, v2,',');

        std::shared_ptr<coyote::Event> res;
            std::cout << "Submitting event of type " << type << " for address " << v2 << " @ " << timestamp << "\n";

        if(type=="l2_read" || type=="memory_read")
        {
            std::shared_ptr<coyote::CacheRequest> ares=createCacheRequest(v2, coyote::CacheRequest::AccessType::LOAD, pc, timestamp, core, v1);
            std::cout << "\tPutting event for address " << ares->getAddress() << "\n";
            res=ares;
        }
        else if(type=="l2_write" || type=="memory_write")
        {
            res=createCacheRequest(v2, coyote::CacheRequest::AccessType::STORE, pc, timestamp, core, v1);
        }
        else
        {
            sparta_assert(false, "Unexpected event type " << type << " found in trace!");
        }
        return res;
}

