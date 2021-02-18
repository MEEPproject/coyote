#ifndef __FIXED_LATENCY_INSTRUCTION_H__
#define __FIXED_LATENCY_INSTRUCTION_H__

#include "InstructionVisitor.hpp"

#include "BaseInstruction.hpp"

namespace spike_model
{
    class FixedLatencyInstruction : public BaseInstruction
    {
        public:
            FixedLatencyInstruction(uint64_t b);
            virtual bool sendToDispatcher(InstructionVisitor& d) override;
        private:
            
    };
    
    //inline std::ostream & operator<<(std::ostream & Str, FixedLatencyInstruction const & i);
}
#endif
