#include "StateInstruction.hpp"

namespace spike_model
{
    StateInstruction::StateInstruction(uint64_t b, Type t) : BaseInstruction::BaseInstruction(b)
    {
        type=t;
        setId(2);
    }
    
    bool StateInstruction::sendToDispatcher(InstructionVisitor& d)
    {
        return d.dispatch(*this);
    }
}
