#ifndef __FIXED_LATENCY_DISPATCHER_H__
#define __FIXED_LATENCY_DISPATCHER_H__

#include "Instructions.hpp"

#include "AbstractDispatcher.hpp"
#include "InstructionVisitor.hpp"

#include "Fetch.hpp"
#include "Execute.hpp"

namespace spike_model
{
    class FixedLatencyDispatcher : public AbstractDispatcher, public InstructionVisitor
    {
        using AbstractDispatcher::AbstractDispatcher;
        public:
            int id; //Just to say something so far
            //FixedLatencyDispatcher();
            
            FixedLatencyDispatcher(sparta::TreeNode * node, const DispatchParameterSet * p);
 
            virtual bool dispatch(MemoryInstruction& i) override;
            virtual bool dispatch(FixedLatencyInstruction& i) override;
            virtual bool dispatch(SynchronizationInstruction& i) override;
            virtual bool dispatch(StateInstruction& i) override;

            void setFetch(spike_model::Fetch * f){fetch_=f;};
            void setExecute(spike_model::Execute * e){execute_=e;};
        protected:
            virtual void dispatchInstructions_() override;

        private:
            spike_model::Fetch * fetch_;
            spike_model::Execute * execute_;
    };
}
#endif
