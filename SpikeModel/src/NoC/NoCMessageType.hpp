
#ifndef __NOC_MESSAGE_TYPE_HH__
#define __NOC_MESSAGE_TYPE_HH__


namespace spike_model
{
    enum class NoCMessageType
    {
        REMOTE_L2_REQUEST,
        MEMORY_REQUEST,
        MEMORY_REQUEST_LOAD,
        MEMORY_REQUEST_STORE,
        REMOTE_L2_ACK,
        MEMORY_ACK,
        MCPU_REQUEST,
        SCRATCHPAD_ACK,
        SCRATCHPAD_DATA_REPLY,
        SCRATCHPAD_COMMAND
    };
}
#endif
