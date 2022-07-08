
#ifndef __MEMORY_HH__
#define __MEMORY_HH__

#include "sparta/ports/PortSet.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/ports/DataPort.hpp"
#include "sparta/events/EventSet.hpp"
#include "sparta/events/UniqueEvent.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/collection/Collectable.hpp"
#include "sparta/events/StartupEvent.hpp"
#include "sparta/resources/Pipeline.hpp"
#include "sparta/resources/Buffer.hpp"
#include "sparta/pairs/SpartaKeyPairs.hpp"
#include "sparta/simulation/State.hpp"
#include "sparta/utils/SpartaSharedPointer.hpp"


#include <memory>

namespace minimum_two_phase_example
{
    class Req;
    class Ack;

    class Memory : public sparta::Unit
    {

        public:

            class MemoryParameterSet : public sparta::ParameterSet
            {
            public:
                MemoryParameterSet(sparta::TreeNode* n):
                    sparta::ParameterSet(n)
                {
                }
                PARAMETER(uint16_t, latency, 70, "The number of l2 cache banks in the tile")
            };

            Memory(sparta::TreeNode* node, const MemoryParameterSet* p);

            ~Memory() {
                debug_logger_ << getContainer()->getLocation()
                              << ": "
                              << std::endl;
            }

            static const char name[];

            void tick();

            void reqReceived(const std::shared_ptr<Req> & req);
            
            
        private:
            uint16_t latency;
            std::queue<std::shared_ptr<Req>> requests_to_handle;
            bool busy;

            //A port to receive requests from the Cache
            sparta::DataInPort<std::shared_ptr<Req>> in_reqs {&unit_port_set_, "in_reqs"};
            
            //A port to send acks to the Cache
            sparta::DataOutPort<std::shared_ptr<Ack>> out_acks {&unit_port_set_, "out_acks", false};

            //This event is used for the operations performed every cycle by the Memory
            sparta::UniqueEvent<sparta::SchedulingPhase::Tick> operation_event_ {&unit_event_set_, "operation_event_", CREATE_SPARTA_HANDLER(Memory, tick)};
            
            //An event to model an access to memory. It is associated to method access
            sparta::PayloadEvent<std::shared_ptr<Req>, sparta::SchedulingPhase::Tick> access_event_ {&unit_event_set_, "access_event_", CREATE_SPARTA_HANDLER_WITH_DATA(Memory, access, std::shared_ptr<Req>)};

            sparta::Counter count_outgoing_acks_=sparta::Counter(getStatisticSet(), "outgoing_acks", "Number of cache requests from local cores", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_incoming_requests_=sparta::Counter(getStatisticSet(), "incoming_reqs", "Number of cache requests from remote cores", sparta::Counter::COUNT_NORMAL);
            
            void access(const std::shared_ptr<Req> & req);
            void checkSchedule();
    };
}
#endif
