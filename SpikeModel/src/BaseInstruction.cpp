#include "BaseInstruction.hpp"


namespace spike_model
{


    BaseInstruction::BaseInstruction(uint64_t b)
    {
        bits=b;
    }

    int BaseInstruction::getId()
    {
        return id;
    }

    void BaseInstruction::setId(int i)
    {
        id=i;
    }
    
    uint8_t BaseInstruction::getOpcode()
    {
        uint8_t res = (uint8_t)bits; //Discards most significant bits
        res <<= 1;
        res >>= 1;
        return res; 
    }

}
