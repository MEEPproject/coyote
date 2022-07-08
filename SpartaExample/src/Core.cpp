
#include "sparta/utils/SpartaAssert.hpp"
#include "sparta/app/Simulation.hpp"
#include "Core.hpp"
#include "Req.hpp"
#include "Ack.hpp"
#include <chrono>

#include <random>

namespace minimum_two_phase_example
{
    const char Core::name[] = "core";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    Core::Core(sparta::TreeNode *node, const CoreParameterSet *p) :
        sparta::Unit(node),
        loads_to_issue(p->loads_to_issue),
        probability(p->probability),
        in_flight_requests()
    {
        //When a message is received through the in_acks port, the notifyAck method from class core will be called. 
        //The message will be interpreted as a std::shared_ptr<Ack>
        in_acks.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Core, notifyAck, std::shared_ptr<Ack>));

        sparta_assert(probability>0 && probability<=1, "The probability to issue should be between 0 and 1");
        sparta_assert(loads_to_issue>0, "At least 1 load should be issued");

        std::random_device rd;

        sparta::StartupEvent(node, CREATE_SPARTA_HANDLER(Core, tick));
    }

    void Core::notifyAck(const std::shared_ptr<Ack> & ack)
    {
        in_flight_requests.erase(ack->getReq()->getAddress());
        count_incoming_acks_++;
    }

    void Core::tick()
    {
        double value=((double) rand() / (RAND_MAX));

        //If the random value is smaller than the probability, issue a load to the cache
        if(value<probability)
        {
            uint64_t address=(uint64_t) value*0xFFFFFFFF;
            std::shared_ptr<Req> r=std::make_shared<Req>(address, 64, Req::AccessType::LOAD);

            //Send the request to the cache with a delay of 1
            out_reqs.send(r,1);
            loads_to_issue--;
            in_flight_requests[address]=r;
            count_outgoing_requests_++;
        }

        //If there are still loads to issue, schedule an event for the next cycle
        if(loads_to_issue>0)
        {
            operation_event_.schedule(1);
        }
    }
}
