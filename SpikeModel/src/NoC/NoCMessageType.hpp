
#ifndef __NOC_MESSAGE_TYPE_HH__
#define __NOC_MESSAGE_TYPE_HH__


namespace spike_model
{
    enum class NoCMessageType
    {
        REMOTE_L2_REQUEST       = 0,
        MEMORY_REQUEST          = 1,
        MEMORY_REQUEST_LOAD     = 2,
        MEMORY_REQUEST_STORE    = 3,
        REMOTE_L2_ACK           = 4,
        MEMORY_ACK              = 5,
        MCPU_REQUEST            = 6,
        SCRATCHPAD_ACK          = 7,
        SCRATCHPAD_DATA_REPLY   = 8,
        SCRATCHPAD_COMMAND      = 9,
        count                   =10 // Number of message types
    };
}
#endif
