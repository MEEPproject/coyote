#ifndef __ABSTRACT_DISPATCHER_H__
#define __ABSTRACT_DISPATCHER_H__


#include "sparta/ports/DataPort.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/events/SingleCycleUniqueEvent.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/log/MessageSource.hpp"
#include "sparta/statistics/Counter.hpp"
#include "sparta/statistics/ContextCounter.hpp"
#include "test/ContextCounter/WeightedContextCounter.hpp"

#include "SpikeModelTypes.hpp"
#include "Fetch.hpp"
#include "Execute.hpp"

#include <iostream>
namespace spike_model
{
    //Forward declarations
    class FixedLatencyInstruction;
    class MemoryInstruction;
    class StateInstruction;
    class SynchronizationInstruction;

    class RegisterAvailability
    {
        public:
            uint8_t reg;
            sparta::Clock::Cycle cycle;

            static const sparta::Clock::Cycle EVENT_DEPENDENT=0;

            RegisterAvailability()
            {
                reg=0;
                cycle=0;
            }

            RegisterAvailability(uint8_t r, sparta::Clock::Cycle c)
            {
                reg=r;
                cycle=c;
            }
            
            bool operator<(RegisterAvailability const &r) const
            {
                return cycle<r.cycle || (cycle==r.cycle && reg<r.reg);
            }
    };    
    class AbstractDispatcher : public sparta::Unit
    {

        public:

            class DispatchParameterSet : public sparta::ParameterSet
            {
                public:
                    DispatchParameterSet(sparta::TreeNode* n) :
                        sparta::ParameterSet(n)
                    { }

                    PARAMETER(uint32_t, num_to_dispatch,       1, "Number of instructions to dispatch")
                    PARAMETER(uint32_t, dispatch_queue_depth, 1, "Depth of the dispatch buffer")
            };

            //AbstractDispatcher();


            AbstractDispatcher(sparta::TreeNode * node,
               const DispatchParameterSet * p);

            static const char name[];
    
            void markRegisterAsAvailable(uint8_t reg);    

        protected:
            bool finished;
            InstQueue dispatch_queue_;

            const uint32_t num_to_dispatch_;

            ///////////////////////////////////////////////////////////////////////
            // Stall counters
            enum StallReason {
                NOT_STALLED,     // Made forward progress (dipatched all instructions or no instructions)
                FIXED_LATENCY_BUSY,        // Could not send any or all instructions -- Fixed Latency busy
                LSU_BUSY,
                RAW,
                WAITING_ON_MEMORY,
                N_STALL_REASONS
            };

            StallReason current_stall_ = NOT_STALLED;

            std::array<sparta::Counter,
                       static_cast<uint32_t>(2)> //BORJA: This 1 is horrible design
            unit_distribution_ {{
                sparta::Counter(getStatisticSet(), "count_fixed_latency_insts",
                              "Total Fixed Latency insts", sparta::Counter::COUNT_NORMAL),
                sparta::Counter(getStatisticSet(), "count_lsu_insts",
                              "Total LSU Insts", sparta::Counter::COUNT_NORMAL)
            }};
            
            uint32_t credits_fixed_latency_ = 0;


            virtual void dispatchInstructions_()=0;

            // Counters -- this is only supported in C++11 -- uses
            // Counter's move semantics
            std::array<sparta::CycleCounter, N_STALL_REASONS> stall_counters_{{
                sparta::CycleCounter(getStatisticSet(), "stall_not_stalled",
                                   "Dispatch not stalled, all instructions dispatched",
                                   sparta::Counter::COUNT_NORMAL, getClock()),
                sparta::CycleCounter(getStatisticSet(), "stall_fixed_latency_busy",
                                   "FixedLatency busy",
                                   sparta::Counter::COUNT_NORMAL, getClock()),
                sparta::CycleCounter(getStatisticSet(), "stall_lsu_busy",
                                   "LSU busy",
                                   sparta::Counter::COUNT_NORMAL, getClock()),
                sparta::CycleCounter(getStatisticSet(), "raw_dependency",
                                   "RAW",
                                   sparta::Counter::COUNT_NORMAL, getClock()),
                sparta::CycleCounter(getStatisticSet(), "waiting_on_memory",
                                   "WAITING_ON_MEMORY",
                                   sparta::Counter::COUNT_NORMAL, getClock())
            }};
            
            sparta::UniqueEvent<> ev_dispatch_insts_{&unit_event_set_, "dispatch_event", CREATE_SPARTA_HANDLER(AbstractDispatcher, dispatchInstructions_)};

            void markRegisterAsUsed(uint8_t reg);

            std::set<RegisterAvailability> register_availability;

            
            bool checkRawAndUpdate(uint8_t rs1, uint8_t rs2, uint8_t rs3, RegisterAvailability &avail);
            uint32_t latency_last_inst_;
            
            std::shared_ptr<BaseInstruction> pending_inst=NULL;
        
            unsigned num_dispatched_since_last_ev;
            uint8_t reg_expected_for_raw;
        private:

    
            // Ports


            uint32_t credits_lsu_ = 0;

            // Tick callbacks assigned to Ports -- zero cycle
            void fixedLatencyCredits_ (const uint32_t&);
            void fixedLatencyForwarding_ (const uint8_t&);
            void lsuCredits_ (const uint32_t&);

            // Dispatch instructions
            void dispatchQueueAppended_(const InstGroup &);
        
            void sendInitialCredits_();


            // ContextCounter with only one context. These are handled differently
            // than other ContextCounters; they are not automatically expanded to
            // include per-context information in reports, since that is redundant
            // information.
            sparta::ContextCounter<sparta::Counter> fixed_latency_context_ {
                getStatisticSet(),
                "context_count_fixed_latency_insts",
                "Fixed Latency instruction count",
                1,
                "dispatch_fixed_latency_inst_count",
                sparta::CounterBase::COUNT_NORMAL,
                sparta::InstrumentationNode::VIS_NORMAL
            };


            sparta::StatisticDef total_insts_{
                getStatisticSet(), "count_total_insts_dispatched",
                "Total number of instructions dispatched",
                getStatisticSet(), "count_fixed_latency_insts + count_lsu_insts"
            };
    };
   
     
    inline std::ostream & operator<<(std::ostream & Str, RegisterAvailability const & avail)
    {
        Str << "(" << unsigned(avail.reg) << ", " << unsigned(avail.cycle) << ")";
        return Str;
    }
}
#endif
