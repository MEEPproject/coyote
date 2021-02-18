// <Fetch.cpp> -*- C++ -*-

//!
//! \file Fetch.cpp
//! \brief Implementation of the CoreModel Fetch unit
//!

#include <algorithm>
#include "Fetch.hpp"

#include "sparta/events/StartupEvent.hpp"

namespace spike_model
{
    const char * Fetch::name = "fetch";

    std::shared_ptr<BaseInstruction> Fetch::fetchInstruction_()
    {
        //printf("Fetch called\n");
        std::shared_ptr<BaseInstruction> ex_inst=spike->getInstruction(core_id_);
        
        return ex_inst;
    }

    Fetch::Fetch(sparta::TreeNode * node,
                 const FetchParameterSet * p) :
        sparta::Unit(node),
        num_insts_to_fetch_(p->num_to_fetch)
    {
    }

    // Called when decode has room
    void Fetch::receiveFetchQueueCredits_(const uint32_t & dat) {
        credits_inst_queue_ += dat;

        if(SPARTA_EXPECT_FALSE(info_logger_)) {
            info_logger_ << "Fetch: receive num_decode_credits=" << dat
                         << ", total decode_credits=" << credits_inst_queue_;
        }

        // Schedule a fetch event this cycle
        fetch_inst_event_->schedule(sparta::Clock::Cycle(0));
    }
    
    void Fetch::setSpikeConnector(SpikeWrapper * s)
    {
        spike=s;
    }

    void Fetch::setCoreId(uint32_t i)
    {
        core_id_=i;
    }

}
