#ifndef __BASE_INSTRUCTION_H__
#define __BASE_INSTRUCTION_H__

#include <stdint.h>
#include <iostream>

namespace spike_model
{
    //Forward declaration
    class InstructionVisitor;

    class BaseInstruction
    {
        public:

            BaseInstruction(uint64_t bits);

            virtual bool sendToDispatcher(InstructionVisitor& d)=0;
    
            int getId();
        
            void setId(int id);

            uint8_t getOpcode();            
          
            uint64_t getBits() const {return bits;};
 
            uint8_t getRd() const {return rd;};
            uint8_t getRs1() const {return rs1;};
            uint8_t getRs2() const {return rs2;};
            uint8_t getRs3() const {return rs3;};
 
            void setRd(uint8_t r) {rd=r;};
            void setRs1(uint8_t r) {rs1=r;};
            void setRs2(uint8_t r) {rs2=r;};
            void setRs3(uint8_t r) {rs3=r;};

        private:
            
            int id=-1;

            uint64_t bits;
            
            uint8_t rd=0;
            uint8_t rs1=0;
            uint8_t rs2=0;
            uint8_t rs3=0;
    };
    
    inline std::ostream & operator<<(std::ostream & Str, BaseInstruction const & i)
    {
        Str << "0x" << std::hex << i.getBits();
        return Str;
    }
}

#endif
