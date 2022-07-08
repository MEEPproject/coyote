
#include "sparta/utils/SpartaAssert.hpp"
#include "sparta/app/Simulation.hpp"
#include "Memory.hpp"
#include "Req.hpp"
#include "Ack.hpp"
#include <chrono>


namespace minimum_two_phase_example
{
    const char Memory::name[] = "memory";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    Memory::Memory(sparta::TreeNode *node, const MemoryParameterSet *p) :
        sparta::Unit(node),
        latency(p->latency),
        requests_to_handle(),
        busy(false)
    {
        //When something is received through the port from the cache, execute method reqReceived
        in_reqs.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Memory, reqReceived, std::shared_ptr<Req>));
    }

    void Memory::reqReceived(const std::shared_ptr<Req> & req)
    {
        requests_to_handle.push(req);
        count_incoming_requests_++;
        if(!busy)
        {
            operation_event_.schedule();
            busy=true;
        }
    }

    void Memory::access(const std::shared_ptr<Req> & req)
    {
        out_acks.send(std::make_shared<Ack>(req),1);
        count_outgoing_acks_++;
    }

    void Memory::tick()
    {
        if(requests_to_handle.size()>0)
        {
            // Method access will be executed latency cycles later using requests_to_handle.front() as an argument
            access_event_.preparePayload(requests_to_handle.front())->schedule(latency);
            requests_to_handle.pop();
        }
        
        checkSchedule();
    }

    void Memory::checkSchedule()
    {
        //Schedule the operation event if there is anything to do this cycle.
        if(requests_to_handle.size()>0)
        {
            operation_event_.schedule(1);
            busy=true;
        }
        else
        {
            busy=false;
        }

    }
}
