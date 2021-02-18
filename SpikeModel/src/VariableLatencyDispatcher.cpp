
#include "VariableLatencyDispatcher.hpp"
namespace spike_model
{
    VariableLatencyDispatcher::VariableLatencyDispatcher(sparta::TreeNode * node, const DispatchParameterSet * p) : FixedLatencyDispatcher::FixedLatencyDispatcher(node, p)
    {

    }
                
    bool VariableLatencyDispatcher::dispatch(MemoryInstruction& i)
    {
        ++unit_distribution_[i.getId()];
        bool hit=lsu_->issueInst_(i);

        if(hit)
        {
            latency_last_inst_=1;
        }
        else
        {
            latency_last_inst_=RegisterAvailability::EVENT_DEPENDENT;
            //printf("Mem on Rd: %u\n", i.getRd());
        }
        return true; //SHOULD BE CHANGES WHEN THE LSU HAS LIMITTED RESOURCES
    }

    bool VariableLatencyDispatcher::dispatch(FixedLatencyInstruction& i)
    {
        return FixedLatencyDispatcher::dispatch(i);
    }

    bool VariableLatencyDispatcher::dispatch(StateInstruction& i)
    {
        return FixedLatencyDispatcher::dispatch(i);
    }

    bool VariableLatencyDispatcher::dispatch(SynchronizationInstruction& i)
    {
        return FixedLatencyDispatcher::dispatch(i);
    }

    void VariableLatencyDispatcher::dispatchInstructions_()
    {
        FixedLatencyDispatcher::dispatchInstructions_();
    }
}
