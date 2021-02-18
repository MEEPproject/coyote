#include "FixedLatencyInstruction.hpp"

namespace spike_model
{
    
    FixedLatencyInstruction::FixedLatencyInstruction(uint64_t b) : BaseInstruction::BaseInstruction(b)
    {
        setId(0); //This should be changed, as it is not a good design. Instruction id should be elsewhere.
    }

    bool FixedLatencyInstruction::sendToDispatcher(InstructionVisitor& d)
    {
        return d.dispatch(*this);
    }
    
    /*std::ostream & operator<<(std::ostream & Str, FixedLatencyInstruction const & i)
    {
        Str << "0x" << std::hex << i.bits;
        return Str;
    }*/
}
