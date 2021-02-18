#ifndef __SYNCHRONIZATION_INSTRUCTION_H__
#define __SYNCHRONIZATION_INSTRUCTION_H__

#include "InstructionVisitor.hpp"

#include "BaseInstruction.hpp"

namespace spike_model
{
    
    class SynchronizationInstruction : public BaseInstruction
    {
        public:
            SynchronizationInstruction(uint64_t b);
            virtual bool sendToDispatcher(InstructionVisitor& d) override;
        private:
    };
}
#endif
