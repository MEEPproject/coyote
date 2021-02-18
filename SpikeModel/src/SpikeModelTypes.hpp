

#ifndef __SPIKE_MODEL_TYPES_H__
#define __SPIKE_MODEL_TYPES_H__

#include <vector>

#include "sparta/resources/Queue.hpp"

#include "BaseInstruction.hpp"

namespace spike_model
{

    typedef std::vector<std::shared_ptr<BaseInstruction>> InstGroup;

    //! Instruction Queue
    typedef sparta::Queue<std::shared_ptr<BaseInstruction>> InstQueue;

    namespace message_categories {
        const std::string INFO = "info";
        // More can be added here, with any identifier...
    }

}

#endif
