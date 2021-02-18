// <Fetch.hpp> -*- C++ -*-

//!
//! \file Fetch.hpp
//! \brief Definition of the CoreModel Fetch unit
//!


#ifndef __FETCH_H__
#define __FETCH_H__

#include <string>
#include "sparta/ports/DataPort.hpp"
#include "sparta/events/SingleCycleUniqueEvent.hpp"
#include "sparta/collection/Collectable.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/simulation/ParameterSet.hpp"

#include "SpikeModelTypes.hpp"
#include "SpikeConnector.hpp"
#include "spike_wrapper.h"

namespace spike_model
{
    /**
     * @file   Fetch.h
     * @brief The Fetch block -- gets new instructions to send down the pipe
     *
     * This fetch unit is pretty simple and does not support
     * redirection.  But, if it did, a port between the ROB and Fetch
     * (or Branch and Fetch -- if we had a Branch unit) would be
     * required to release fetch from holding out on branch
     * resolution.
     */
    class Fetch : public sparta::Unit
    {
    public:
        //! \brief Parameters for Fetch model
        class FetchParameterSet : public sparta::ParameterSet
        {
        public:
            FetchParameterSet(sparta::TreeNode* n) :
                sparta::ParameterSet(n)
            {
                auto non_zero_validator = [](uint32_t & val, const sparta::TreeNode*)->bool {
                    if(val > 0) {
                        return true;
                    }
                    return false;
                };
                num_to_fetch.addDependentValidationCallback(non_zero_validator,
                                                            "Num to fetch must be greater than 0");
            }

            PARAMETER(uint32_t, num_to_fetch, 1, "Number of instructions to fetch")
        };

        /**
         * @brief Constructor for Fetch
         *
         * @param node The node that represents (has a pointer to) the Fetch
         * @param p The Fetch's parameter set
         */
        Fetch(sparta::TreeNode * name,
              const FetchParameterSet * p);

        ~Fetch() {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << " ExampleInst objects allocated/created"
                          << std::endl;
        }

        //! \brief Name of this resource. Required by sparta::UnitFactory
        static const char * name;

        void setSpikeConnector(SpikeWrapper * s);

        void setCoreId(uint32_t i);
        
        std::shared_ptr<BaseInstruction> fetchInstruction_();

    private:
        
 
        SpikeWrapper * spike; //BORJA: should change this to a reference once I have a greater grasp of Factories and how to pass parameters to them
        
        // Number of instructions to fetch
        const uint32_t num_insts_to_fetch_;
        
        uint32_t core_id_;       

        // Number of credits that fetch has
        uint32_t credits_inst_queue_ = 0;

        // Current "PC"
        uint64_t vaddr_ = 0x1000;

        // Fetch instruction event, triggered when there are credits
        // The callback set is to fecth and instruction from spike
        std::unique_ptr<sparta::SingleCycleUniqueEvent<>> fetch_inst_event_;

        ////////////////////////////////////////////////////////////////////////////////
        // Callbacks

        // Receive the number of free credits from the execution of a fixed latency instruction
        void receiveFetchQueueCredits_(const uint32_t &);


        // A unique instruction ID
        uint64_t next_inst_id_ = 0;
    };

}
//__FETCH_H__
#endif
