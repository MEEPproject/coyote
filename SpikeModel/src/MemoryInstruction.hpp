#ifndef __MEMORY_INSTRUCTION_H__
#define __MEMORY_INSTRUCTION_H__

#include <vector>

#include "InstructionVisitor.hpp"

#include "BaseInstruction.hpp"

namespace spike_model
{
    
    struct memory_access_t
    {
        uint64_t address;
        uint8_t size;
    };

    class MemoryInstruction : public BaseInstruction
    {
        public:

            MemoryInstruction(uint64_t b);
            
            void addMemoryAccess(uint64_t address, uint8_t size);
            
            virtual bool sendToDispatcher(InstructionVisitor& d) override;

            std::vector<memory_access_t> getAccesses(){return accesses;};

        private:
            std::vector<memory_access_t> accesses;
    };
}
#endif
