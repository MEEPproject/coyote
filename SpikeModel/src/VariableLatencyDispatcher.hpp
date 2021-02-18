#ifndef __VARIABLE_LATENCY_DISPATCHER_H__
#define __VARIABLE_LATENCY_DISPATCHER_H__

#include "FixedLatencyDispatcher.hpp"
#include "LSU.hpp"

namespace spike_model
{
    class LSU;
    class VariableLatencyDispatcher : public FixedLatencyDispatcher 
    {
        public:
            VariableLatencyDispatcher(sparta::TreeNode * node, const DispatchParameterSet * p);
            
            virtual bool dispatch(MemoryInstruction& i) override;
            virtual bool dispatch(FixedLatencyInstruction& i) override;
            virtual bool dispatch(SynchronizationInstruction& i) override;
            virtual bool dispatch(StateInstruction& i) override;
        
            void setLSU(spike_model::LSU * l){lsu_=l;};

        protected:
            virtual void dispatchInstructions_() override;
        
        private:
            spike_model::LSU * lsu_;
            unsigned long d=0;
    };    
}
#endif
