
#include "sparta/utils/SpartaAssert.hpp"
#include "sparta/app/Simulation.hpp"
#include "Cache.hpp"
#include "Req.hpp"
#include "Ack.hpp"
#include <chrono>


namespace minimum_two_phase_example
{
    const char Cache::name[] = "cache";

    Cache::Cache(sparta::TreeNode *node, const CacheParameterSet *p) :
        sparta::Unit(node),
        latency(p->latency),
        hit_probability(p->hit_probability),
        requests_to_handle(),
        busy(false)
    {
        //When something is received through the port from memory, execute method ackReceived 
        in_acks.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Cache, ackReceived, std::shared_ptr<Ack>));
        
        //When something is received through the port from the Core, execute method reqReceived
        in_reqs.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Cache, reqReceived, std::shared_ptr<Req>));
    }

    void Cache::sendAck()
    {
        out_acks.send(acks_to_send.front(),1);
        acks_to_send.pop();
    }

    void Cache::ackReceived(const std::shared_ptr<Ack> & ack)
    {
        //Add the ack to the list and schedule the operation event if the Cache is idle
        count_incoming_acks_++;
        acks_to_send.push(ack);
        if(!busy)
        {
            operation_event_.schedule();
            busy=true;
        }
    }

    void Cache::reqReceived(const std::shared_ptr<Req> & req)
    {
        //Add the request to the queue of requests to handle
        requests_to_handle.push(req);

        //Schedule the operation event if the Cache is idle
        if(!busy)
        {
            operation_event_.schedule();
            busy=true;
        }
    }

    void Cache::lookup(const std::shared_ptr<Req> & req)
    {
        double value=((double) rand() / (RAND_MAX));

        //If it is a hit (just random actually), send an ack. Send a memory request otherwise.
        if(value<hit_probability)
        {
            acks_to_send.push(std::make_shared<Ack>(req));
        }
        else
        {
            count_outgoing_requests_++;
            out_reqs.send(req,1);
        }

        if(!busy)
        {
            checkSchedule();
        }
    }

    void Cache::tick()
    {
        if(requests_to_handle.size()>0)
        {
            //Issue a lookup event. The lookup method will be called with a delay of "latency" and requests_to_handle.front() as its argument
            lookup_event_.preparePayload(requests_to_handle.front())->schedule(latency);
            requests_to_handle.pop();
        }

        if(acks_to_send.size()>0)
        {
            sendAck();
        }
        
        checkSchedule();
    }

    void Cache::checkSchedule()
    {
        //If there is anything to do, issue an operation event for the next cycle
        if(requests_to_handle.size()>0 || acks_to_send.size()>0)
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
