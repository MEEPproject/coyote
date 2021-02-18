// <Execute.cpp> -*- C++ -*-

#include "sparta/utils/SpartaAssert.hpp"

#include <map>

#include "Execute.hpp"

namespace spike_model
{
    const char Execute::name[] = "execute";
    Execute::Execute(sparta::TreeNode * node,
                     const ExecuteParameterSet * p) :
        sparta::Unit(node),
        scheduler_size_(p->scheduler_size),
        in_order_issue_(p->in_order_issue)
        //collected_inst_(node, node->getName())
    {

    }

    void Execute::sendInitialCredits_()
    {
    }

    uint32_t Execute::issueInst_(FixedLatencyInstruction& ex_inst) {
        // Issue a random instruction from the ready queue
        // Issue the first instruction
        const uint32_t exe_time = getExeTime(ex_inst);

        //collected_inst_.collectWithDuration(ex_inst, exe_time);
        if(SPARTA_EXPECT_FALSE(info_logger_)) {
            info_logger_ << "Executing: " << ex_inst << " for "
                         << exe_time + getClock()->currentCycle();
        }
        sparta_assert(exe_time != 0);

        ++total_insts_issued_;
        
        return exe_time;
    }

        
    uint32_t Execute::getExeTime(FixedLatencyInstruction& i)
    {
        uint32_t latency=1;
        std::map<uint8_t, uint32_t>::iterator it=instruction_latencies_.find(i.getOpcode());
        if(it != instruction_latencies_.end())
        {
            latency=it->second;
            //std::cout << "Instruction " << i << "is latency " << latency << " and has opcode\n";
            //printf("---------> Opcode %u\n", i.getOpcode());
        }
        return latency;
    }
}
