#include "SynchronizationInstruction.hpp"

namespace spike_model
{
    SynchronizationInstruction::SynchronizationInstruction(uint64_t b) : BaseInstruction::BaseInstruction(b)
    {
        setId(2);
    }
    
    bool SynchronizationInstruction::sendToDispatcher(InstructionVisitor& d)
    {
        return d.dispatch(*this);
    }
}
