#include <sstream>
#include <string>

#include "TraceDrivenSimulationOrchestrator.hpp"
#include "CacheRequest.hpp"


TraceDrivenSimulationOrchestrator::TraceDrivenSimulationOrchestrator(std::string trace_path, std::shared_ptr<SpikeModel>& spike_model, spike_model::SimulationEntryPoint * entry_point, bool trace, spike_model::NoC* noc) :
    spike_model(spike_model),
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
    spike_model->saveReports();
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
            std::shared_ptr<spike_model::Event> event=parse(line);
            if(event->getTimestamp()>current_cycle)
            {
                uint64_t cycles=event->getTimestamp()-spike_model->getScheduler()->getCurrentTick(); //runRaw not inclusive. All events prior to the one in the timestamp handled
                std::cout << "Advancing " << cycles << " cycles starting from t " << spike_model->getScheduler()->getCurrentTick() << "\n";
                spike_model->runRaw(cycles);
                std::cout << "Now the clock is " << spike_model->getScheduler()->getCurrentTick() << "\n";
                current_cycle=event->getTimestamp();
            }
            entry_point->putEvent(event);
        }
    }

    std::cout << "After 1st loop\n";

 //   uint64_t next_event_tick=spike_model->getScheduler()->nextEventTick();

    //Handle pending events
    spike_model->run(sparta::Scheduler::INDEFINITE);
    /*while(next_event_tick!=sparta::Scheduler::INDEFINITE)
    {
        uint64_t cycles=next_event_tick-spike_model->getScheduler()->getCurrentTick()+1; //runRaw not inclusive. All events handled included the one in next_event_tick
        std::cout << "Next is " <<  next_event_tick << ", current is " << spike_model->getScheduler()->getCurrentTick() << ". Advancing " << cycles << " cycles starting from t " << spike_model->getScheduler()->getCurrentTick() << "\n";
        spike_model->runRaw(cycles);
        next_event_tick=spike_model->getScheduler()->nextEventTick();
    }*/
    std::cout << "After 2nd loop\n";
    //spike_model->saveReports();
}

std::shared_ptr<spike_model::CacheRequest> TraceDrivenSimulationOrchestrator::createCacheRequest(std::string& address, spike_model::CacheRequest::AccessType t, std::string& pc, std::string& timestamp, std::string& core, std::string& size)
{
    std::shared_ptr<spike_model::CacheRequest> req=std::make_shared<spike_model::CacheRequest>(std::stoull(address.c_str(), nullptr, 16), t, std::stoull(pc.c_str(), nullptr, 16), std::stoull(timestamp.c_str()), std::stoi(core.c_str()));
    req->setSize(std::stoi(size.c_str()));
    return req;
}

std::shared_ptr<spike_model::Event> TraceDrivenSimulationOrchestrator::parse(std::string& line)
{
        std::string timestamp, core, pc, type, v1, v2;
        std::istringstream iss(line);
        getline(iss, timestamp,',');
        getline(iss, core,',');
        getline(iss, pc,',');
        getline(iss, type,',');
        getline(iss, v1,',');
        getline(iss, v2,',');

        std::shared_ptr<spike_model::Event> res;
            std::cout << "Submitting event of type " << type << " for address " << v2 << " @ " << timestamp << "\n";

        if(type=="l2_read" || type=="memory_read")
        {
            std::shared_ptr<spike_model::CacheRequest> ares=createCacheRequest(v2, spike_model::CacheRequest::AccessType::LOAD, pc, timestamp, core, v1);
            std::cout << "\tPutting event for address " << ares->getAddress() << "\n";
            res=ares;
        }
        else if(type=="l2_write" || type=="memory_write")
        {
            res=createCacheRequest(v2, spike_model::CacheRequest::AccessType::STORE, pc, timestamp, core, v1);
        }
        else
        {
            sparta_assert(false, "Unexpected event type " << type << " found in trace!");
        }
        return res;
}

