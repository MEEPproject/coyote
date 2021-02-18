// <Execute.h> -*- C++ -*-


/**
 * @file   Execute.h
 * @brief
 *
 *
 */

#ifndef __EXECUTE_H__
#define __EXECUTE_H__

#include "sparta/ports/PortSet.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/ports/DataPort.hpp"
#include "sparta/events/EventSet.hpp"
#include "sparta/events/UniqueEvent.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/Clock.hpp"
#include "sparta/ports/Port.hpp"
#include "sparta/collection/Collectable.hpp"
#include "sparta/events/StartupEvent.hpp"

#include "FixedLatencyInstruction.hpp"
#include "SpikeModelTypes.hpp"

namespace spike_model
{
    /**
     * @class Execute
     * @brief
     */
    class Execute : public sparta::Unit
    {

    public:
        //! \brief Parameters for Execute model
        class ExecuteParameterSet : public sparta::ParameterSet
        {
        public:
            ExecuteParameterSet(sparta::TreeNode* n) :
                sparta::ParameterSet(n)
            { }
            PARAMETER(uint32_t, scheduler_size, 8, "Scheduler queue size")
            PARAMETER(bool, in_order_issue, true, "Force in order issue")
        };

        /**
         * @brief Constructor for Execute
         *
         * @param node The node that represents (has a pointer to) the Execute
         * @param p The Execute's parameter set
         */
        Execute(sparta::TreeNode * node,
            const ExecuteParameterSet * p);

        //! \brief Name of this resource. Required by sparta::UnitFactory
        static const char name[];

        uint32_t issueInst_(FixedLatencyInstruction& ex_inst);
    private:

        std::map<uint8_t, uint32_t> instruction_latencies_={
            {67, 2}, //FMA
            {28, 10}, //LOAD
            {24, 10} //LOAD
        };

        // Ports and the set -- remove the ", 1" to experience a DAG issue!

        // Ready queue
        typedef std::list<std::shared_ptr<FixedLatencyInstruction>> ReadyQueue;
        ReadyQueue  ready_queue_;

        // busy signal for the attached alu
        bool unit_busy_ = false;
        
        const uint32_t scheduler_size_;
        const bool in_order_issue_;
        sparta::collection::IterableCollector<std::list<std::shared_ptr<FixedLatencyInstruction>>>
        ready_queue_collector_ {getContainer(), "scheduler_queue",
                ready_queue_, scheduler_size_};


        // A pipeline collector
        //sparta::collection::Collectable<std::shared_ptr<FixedLatencyInstruction>> collected_inst_;

        // Counter
        sparta::Counter total_insts_issued_{
            getStatisticSet(), "total_insts_issued",
            "Total instructions issued", sparta::Counter::COUNT_NORMAL
        };
        sparta::Counter total_insts_executed_{
            getStatisticSet(), "total_insts_executed",
            "Total instructions executed", sparta::Counter::COUNT_NORMAL
        };

        void sendInitialCredits_();
        ////////////////////////////////////////////////////////////////////////////////
        // Callbacks
        void getInstsFromDispatch_(const std::shared_ptr<FixedLatencyInstruction>&);


        uint32_t getExeTime(FixedLatencyInstruction& i);

        // Used to flush the ALU
    };
} // namespace spike_model

//__EXECUTE_H__
#endif
