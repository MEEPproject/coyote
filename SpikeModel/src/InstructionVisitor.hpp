#ifndef __INSTRUCTION_VISITOR_H__
#define __INSTRUCTION_VISITOR_H__

namespace spike_model
{
    //Forward declarations
    class FixedLatencyInstruction;
    class MemoryInstruction;
    class StateInstruction;
    class SynchronizationInstruction;

    class InstructionVisitor
    { 
        public:
            virtual bool dispatch(FixedLatencyInstruction& i)=0;
            virtual bool dispatch(MemoryInstruction& i)=0;
            virtual bool dispatch(StateInstruction& i)=0;
            virtual bool dispatch(SynchronizationInstruction& i)=0;
    };
}
#endif
