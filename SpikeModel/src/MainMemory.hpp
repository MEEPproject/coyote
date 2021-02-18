
#pragma once

#include "sparta/ports/PortSet.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/ports/DataPort.hpp"
#include "sparta/events/EventSet.hpp"
#include "sparta/events/UniqueEvent.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/collection/Collectable.hpp"
#include "sparta/events/StartupEvent.hpp"
#include "sparta/ports/SyncPort.hpp"
#include "sparta/resources/Pipe.hpp"

#include "L2Request.hpp"

namespace spike_model
{
    class MainMemory : public sparta::Unit
    {
    public:
        //! Parameters for MainMemory model
        class MainMemoryParameterSet : public sparta::ParameterSet
        {
        public:
            // Constructor for MainMemoryParameterSet
            MainMemoryParameterSet(sparta::TreeNode* n):
                sparta::ParameterSet(n)
            { }

            PARAMETER(uint32_t, mss_latency, 100, "MainMemory access latency")
        };

        // Constructor for MainMemory
        // node parameter is the node that represent the MainMemory and p is the MainMemory parameter set
        MainMemory(sparta::TreeNode* node, const MainMemoryParameterSet* p);

        // name of this resource.
        static const char name[];


        ////////////////////////////////////////////////////////////////////////////////
        // Type Name/Alias Declaration
        ////////////////////////////////////////////////////////////////////////////////


    private:
        ////////////////////////////////////////////////////////////////////////////////
        // Input Ports
        ////////////////////////////////////////////////////////////////////////////////

        sparta::DataInPort<L2Request> in_mss_req_
            {&unit_port_set_, "in_mss_req_sync", getClock()};


        ////////////////////////////////////////////////////////////////////////////////
        // Output Ports
        ////////////////////////////////////////////////////////////////////////////////

        sparta::DataOutPort<bool> out_mss_ack_sync_
            {&unit_port_set_, "out_mss_ack_sync", getClock()};


        ////////////////////////////////////////////////////////////////////////////////
        // Internal States
        ////////////////////////////////////////////////////////////////////////////////
        const uint32_t mss_latency_;
        bool mss_busy_ = false;


        ////////////////////////////////////////////////////////////////////////////////
        // Event Handlers
        ////////////////////////////////////////////////////////////////////////////////

        // Event to handle MainMemory request from BIU
        sparta::UniqueEvent<> ev_handle_mss_req_
            {&unit_event_set_, "handle_mss_req", CREATE_SPARTA_HANDLER(MainMemory, handle_req_)};


        ////////////////////////////////////////////////////////////////////////////////
        // Callbacks
        ////////////////////////////////////////////////////////////////////////////////

        // Receive new MainMemory request from BIU
        void getReqFromBIU_(std::shared_ptr<BaseInstruction>);

        // Handle MainMemory request
        void handle_req_();


        ////////////////////////////////////////////////////////////////////////////////
        // Regular Function/Subroutine Call
        ////////////////////////////////////////////////////////////////////////////////


    };
}

