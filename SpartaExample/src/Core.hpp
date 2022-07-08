
#ifndef __CORE_HH__
#define __CORE_HH__

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
#include <map>

namespace minimum_two_phase_example
{

    class Req;
    class Ack;

    class Core : public sparta::Unit
    {
        public:

            class CoreParameterSet : public sparta::ParameterSet
            {
            public:
                CoreParameterSet(sparta::TreeNode* n):
                    sparta::ParameterSet(n)
                {
                }
                PARAMETER(uint16_t, loads_to_issue, 1000, "The number of l2 cache banks in the tile")
                PARAMETER(double, probability, 0.5, "The number of l2 cache banks in the tile")
            };

            Core(sparta::TreeNode* node, const CoreParameterSet* p);

            ~Core() {
                debug_logger_ << getContainer()->getLocation()
                              << ": "
                              << std::endl;
            }

            static const char name[];

            void tick();

            void notifyAck(const std::shared_ptr<Ack> & ack);
            
            
        private:
            uint16_t loads_to_issue;
            double probability;
        
            std::unordered_map<std::uint64_t, std::shared_ptr<Req>> in_flight_requests;

            //A port to receive acks from the Cache
            sparta::DataInPort<std::shared_ptr<Ack>> in_acks {&unit_port_set_, "in_acks"};

            //A port to send requests to the Cache
            sparta::DataOutPort<std::shared_ptr<Req>> out_reqs {&unit_port_set_, "out_reqs", false};

            //This event is used for the operations performed every cycle by the core
            sparta::UniqueEvent<sparta::SchedulingPhase::Tick> operation_event_ {&unit_event_set_, "operation_event_", CREATE_SPARTA_HANDLER(Core, tick)};

            sparta::Counter count_outgoing_requests_=sparta::Counter(getStatisticSet(), "outgoing_requests", "Number of cache requests from local cores", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_incoming_acks_=sparta::Counter(getStatisticSet(), "incoming_acks", "Number of cache requests from remote cores", sparta::Counter::COUNT_NORMAL);
    };
}
#endif
