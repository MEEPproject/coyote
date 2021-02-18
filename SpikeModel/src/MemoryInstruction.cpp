#include "MemoryInstruction.hpp"

namespace spike_model
{
    MemoryInstruction::MemoryInstruction(uint64_t b) : BaseInstruction::BaseInstruction(b)
    {
        setId(1);
    }

    bool MemoryInstruction::sendToDispatcher(InstructionVisitor& d) 
    {
        return d.dispatch(*this);
    }
    
    void MemoryInstruction::addMemoryAccess(uint64_t address, uint8_t size)
    {
        memory_access_t a={address, size};
        accesses.push_back(a);
    }
}
