#ifndef __MEMORY__ACCESS_ORCHESTRATOR__H__
#define __MEMORY__ACCESS_ORCHESTRATOR__H__

#include "sparta/ports/PortSet.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/ports/DataPort.hpp"
#include "sparta/events/EventSet.hpp"
#include "sparta/events/UniqueEvent.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/Clock.hpp"
#include "sparta/ports/Port.hpp"
#include "sparta/collection/Collectable.hpp"
#include "sparta/events/StartupEvent.hpp"

#include "cache/TreePLRUReplacement.hpp"
#include "FixedLatencyInstruction.hpp"
#include "SpikeModelTypes.hpp"
#include "SimpleTLB.hpp"
#include "NoC.hpp"

#include "spike_wrapper.h"

namespace spike_model
{
    class NoC; //Forward declaration

    class Core : public sparta::Unit
    {
        public:
            //! \brief Parameters for Execute model
            class CoreParameterSet : public sparta::ParameterSet
            {
                public:
                    CoreParameterSet(sparta::TreeNode* n) :
                        sparta::ParameterSet(n)
                    { }
                    PARAMETER(uint64_t, latency_to_l2, 1, "The latency to l2")
                    //PARAMETER(uint32_t, scheduler_size, 8, "Scheduler queue size")
            };

            Core(sparta::TreeNode * node,
                const CoreParameterSet * p);

            //! \brief Name of this resource. Required by sparta::UnitFactory
            static const char name[];

            void setSpike(SpikeWrapper& s)
            {
                spike=&s;
            }

            void setNoC(NoC& n)
            {
                noc=&n;
            }

            void setId(uint16_t i)
            {
                id_=i;
            }    

            void ack_(const L2Request & access);

        private:
            bool finished_;
            
            std::list<std::shared_ptr<spike_model::L2Request>> pending_misses_;
            bool running_;

            uint64_t latency_to_l2_;
 
            SpikeWrapper * spike;

            void getMisses_();
            void handleMiss_(std::shared_ptr<spike_model::L2Request> miss);
            void startup_();

            NoC * noc;
            
            uint16_t id_;

            sparta::UniqueEvent<> simulate_inst_event_ 
                {&unit_event_set_, "simulate_inst_", CREATE_SPARTA_HANDLER(Core, getMisses_)};
        
            sparta::Counter count_l2_requests_=sparta::Counter(getStatisticSet(), "l2_requests", "Number of requests", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_dependency_stalls_=sparta::Counter(getStatisticSet(), "dependency_stalls", "Number of stalls due to data dependencies", sparta::Counter::COUNT_NORMAL);
    };
}
#endif
