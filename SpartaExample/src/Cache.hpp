
#ifndef __CACHE_HH__
#define __CACHE_HH__

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

    class Cache : public sparta::Unit
    {
        public:
            class CacheParameterSet : public sparta::ParameterSet
            {
            public:
                CacheParameterSet(sparta::TreeNode* n):
                    sparta::ParameterSet(n)
                {
                }
                PARAMETER(uint16_t, latency, 10, "The number of l2 cache banks in the tile")
                PARAMETER(double, hit_probability , 0.8, "The number of l2 cache banks in the tile")
            };

            Cache(sparta::TreeNode* node, const CacheParameterSet* p);

            ~Cache() {
                debug_logger_ << getContainer()->getLocation()
                              << ": "
                              << std::endl;
            }

            static const char name[];

            void tick();

            void ackReceived(const std::shared_ptr<Ack> & ack);
            
            void reqReceived(const std::shared_ptr<Req> & req);
            
            
        private:
            uint16_t latency;
            double hit_probability;

            std::queue<std::shared_ptr<Req>> requests_to_handle;
            std::queue<std::shared_ptr<Ack>> acks_to_send;
            bool busy;

            //A port to receive acks from Memory 
            sparta::DataInPort<std::shared_ptr<Ack>> in_acks {&unit_port_set_, "in_acks"};

            //A port to send acks to the Core 
            sparta::DataOutPort<std::shared_ptr<Ack>> out_acks {&unit_port_set_, "out_acks", false};

            //A port to receive requests from the Core
            sparta::DataInPort<std::shared_ptr<Req>> in_reqs {&unit_port_set_, "in_reqs"};

            //A port to send requests to the Memory
            sparta::DataOutPort<std::shared_ptr<Req>> out_reqs {&unit_port_set_, "out_reqs", false};

            //This event is used for the operations performed every cycle by the Cache
            sparta::UniqueEvent<sparta::SchedulingPhase::Tick> operation_event_ {&unit_event_set_, "operation_event_", CREATE_SPARTA_HANDLER(Cache, tick)};

            //An event to model the lookup in the cache. It is associated to method lookup
            sparta::PayloadEvent<std::shared_ptr<Req>, sparta::SchedulingPhase::Tick> lookup_event_ {&unit_event_set_, "lookup_event_", CREATE_SPARTA_HANDLER_WITH_DATA(Cache, lookup, std::shared_ptr<Req>)};

            sparta::Counter count_outgoing_requests_=sparta::Counter(getStatisticSet(), "outgoing_requests", "Number of cache requests from local cores", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_incoming_acks_=sparta::Counter(getStatisticSet(), "incoming_acks", "Number of cache requests from remote cores", sparta::Counter::COUNT_NORMAL);
            
            void lookup(const std::shared_ptr<Req> & req);
            void checkSchedule();
            void sendAck();
    };
}
#endif
