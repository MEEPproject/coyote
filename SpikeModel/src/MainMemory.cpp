
#include "sparta/utils/SpartaAssert.hpp"
#include "MainMemory.hpp"

namespace spike_model
{
    const char MainMemory::name[] = "main_mem";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    MainMemory::MainMemory(sparta::TreeNode *node, const MainMemoryParameterSet *p) :
        sparta::Unit(node),
        mss_latency_(p->mss_latency)
    {
        in_mss_req_.registerConsumerHandler
            (CREATE_SPARTA_HANDLER_WITH_DATA(MainMemory, getReqFromBIU_, L2Request));


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "MainMemory construct: #" << node->getGroupIdx();
        }
    }


    ////////////////////////////////////////////////////////////////////////////////
    // Callbacks
    ////////////////////////////////////////////////////////////////////////////////

    // Receive new MainMemory request from BIU
    void MainMemory::getReqFromBIU_(const L2Request & inst_ptr)
    {
        sparta_assert((inst_ptr != nullptr), "MainMemory is not handling a valid request!");

        // Handle MainMemory request event can only be scheduled when MMS is not busy
        if (!mss_busy_) {
            mss_busy_ = true;
            ev_handle_mss_req_.schedule(mss_latency_);
        }
        else {
            // Assumption: MainMemory can handle a single request each time
            sparta_assert(false, "MainMemory can never receive requests from BIU when it's busy!");
        }

        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "MainMemory is busy servicing your request......";
        }
    }

    // Handle MainMemory request
    void MainMemory::handle_memory_req_()
    {
        mss_busy_ = false;
        out_mss_ack_sync_.send(true);

        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "MainMemory is done!";
        }
    }


    ////////////////////////////////////////////////////////////////////////////////
    // Regular Function/Subroutine Call
    ////////////////////////////////////////////////////////////////////////////////


}
