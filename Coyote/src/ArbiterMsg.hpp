#ifndef __ARBITER_MESSAGE_HH__
#define __ARBITER_MESSAGE_HH__

namespace coyote
{

enum class MessageType
{
    NOC_MSG,
    CACHE_REQUEST
};

typedef struct ArbiterMessage
{
    std::shared_ptr<void> msg;
    bool is_core;
    int id;
    MessageType type;
}ArbiterMessage;

}
#endif
