#ifndef __STATE_INSTRUCTION_H__
#define __STATE_INSTRUCTION_H__


#include "InstructionVisitor.hpp"

#include "BaseInstruction.hpp"

namespace spike_model
{ 
    class StateInstruction : public BaseInstruction
    {
        public:
            enum class Type
            {
               FINISH=0 
            };

            StateInstruction(uint64_t b, Type t);
            
            virtual bool sendToDispatcher(InstructionVisitor& d) override;

            StateInstruction::Type getType(){return type;}; 
            
        private:
            Type type;
    };
}
#endif
