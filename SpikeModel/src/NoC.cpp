

#include "sparta/utils/SpartaAssert.hpp"
#include "NoC.hpp"
#include <chrono>

namespace spike_model
{
    const char NoC::name[] = "noc";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    NoC::NoC(sparta::TreeNode *node, const NoCParameterSet *p) :
        sparta::Unit(node),
        num_cores_(p->num_cores),
        cores_(num_cores_)
    {

        in_l2_ack_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(NoC, issueAck_, std::shared_ptr<L2Request>));
    }

    void NoC::send_(const std::shared_ptr<L2Request> & req)
    {
        out_l2_req_.send(req, 0); 
    }

    void NoC::issueAck_(const std::shared_ptr<L2Request> & req)
    {
        cores_[req->getCoreId()]->ack_(req);
    }
}
