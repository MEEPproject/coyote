
#ifndef __MEMORY_CONTROLLER_H__
#define __MEMORY_CONTROLLER_H__

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
#include "sparta/resources/Pipeline.hpp"
#include "sparta/resources/Buffer.hpp"
#include "sparta/pairs/SpartaKeyPairs.hpp"
#include "sparta/simulation/State.hpp"
#include "sparta/utils/SpartaSharedPointer.hpp"

#include <memory>
#include <queue>

#include "LogCapable.hpp"
#include "MemoryAccessSchedulerIF.hpp"
#include "CommandSchedulerIF.hpp"
#include "MemoryBank.hpp"
#include "AddressMappingPolicy.hpp"

namespace spike_model
{
    class MemoryBank; //Forward declaration
    class MemoryAccessSchedulerIF; //Forward declaration

    class MemoryController : public sparta::Unit, public LogCapable
    {
        /*!
         * \class spike_model::MemoryController
         * \brief MemoryController models a the operation of simple memory controller
         *
         * To make modification and extension easy, many functionalities related to the scheduling of commands/requests
         * have been extracted to other classes, such as MemoryAccessSchedulerIF. A reduced subset of commands is supported.
         * The addition of new commands is expected to involve limited changes to controllerCycle_() and notifyCompletion_() methods.
         */
        public:
            /*!
             * \class MemoryControllerParameterSet
             * \brief Parameters for MemoryController model
             */
            class MemoryControllerParameterSet : public sparta::ParameterSet
            {
            public:
                //! Constructor for MemoryControllerParameterSet
                MemoryControllerParameterSet(sparta::TreeNode* n):
                    sparta::ParameterSet(n)
                {
                }
                PARAMETER(uint64_t, num_banks, 8, "The number of banks handled by this memory controller")
                PARAMETER(bool, write_allocate, true, "The write allocation policy")
                PARAMETER(std::string, reordering_policy, "none", "Request reordering policy")
                PARAMETER(std::string, address_policy, "close_page", "Request reordering policy")
            };

            /*!
             * \brief Constructor for MemoryController
             * \param node The node that represent the MemoryController and
             * \param p The MemoryController parameter set
             */
            MemoryController(sparta::TreeNode* node, const MemoryControllerParameterSet* p);

            ~MemoryController() {
                debug_logger_ << getContainer()->getLocation()
                              << ": "
                              << std::endl;
            }

            //! name of this resource.
            static const char name[];

            /*!
             * \brief Associate a bank to the memory controller
             * \param bank The bank
             */
            void addBank_(MemoryBank * bank);

            /*!
             * \brief Notify the completion of a command
             * \param c The command
             */
            void notifyCompletion_(std::shared_ptr<BankCommand> c);
 
            /*!
             * \brief Notify that a timing event has been fulfilled, which might enable a new request to be selected for submission by the scheduler
             */
            void notifyTimingEvent();

            /*!
             * \brief Set up the masks and shifts to identify where the data is in the memory.
             * \brief The number of memory controllers
             * \param The number of rows in each bank
             * \param The number of columns in each bank
             * \param The size of a cache line
             */
            void setup_masks_and_shifts_(uint64_t num_mcs, uint64_t num_rows_per_bank, uint64_t num_cols_per_bank, uint16_t line_size);

            /*
             *\brief Get the AddressMappingPolicy for the memory controller
             *\return The address mapping policy
             */
            spike_model::AddressMappingPolicy getAddressMapping();

        private:

            sparta::DataOutPort<std::shared_ptr<CacheRequest>> out_port_mcpu_
                {&unit_port_set_, "out_mcpu", 1};

            sparta::DataInPort<std::shared_ptr<CacheRequest>> in_port_mcpu_
                {&unit_port_set_, "in_mcpu"};

            sparta::UniqueEvent<sparta::SchedulingPhase::PostTick> controller_cycle_event_
                {&unit_event_set_, "controller_cycle_", CREATE_SPARTA_HANDLER(MemoryController, controllerCycle_)};

            uint64_t num_banks_;

            bool write_allocate_;
 
            std::string reordering_policy_;
            spike_model::AddressMappingPolicy address_mapping_policy_;

            bool idle_=true;

            std::shared_ptr<std::vector<MemoryBank *>> banks;
            
            uint64_t rank_shift;
            uint64_t bank_shift;
            uint64_t row_shift;
            uint64_t col_shift;

            uint64_t rank_mask;
            uint64_t bank_mask;
            uint64_t row_mask;
            uint64_t col_mask;

            uint16_t line_size;

            std::unique_ptr<MemoryAccessSchedulerIF> sched;
            std::unique_ptr<CommandSchedulerIF> ready_commands;

            sparta::Counter count_read_requests_=sparta::Counter(getStatisticSet(), "read_requests", "Number of read requests (FETCH and LOAD)", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_write_requests_=sparta::Counter(getStatisticSet(), "write_requests", "Number of read requests (STORE & WB)", sparta::Counter::COUNT_NORMAL);
            sparta::Counter total_time_spent_by_read_requests_=sparta::Counter(getStatisticSet(), "total_time_spent_by_read_requests", "The total time spent by read requests", sparta::Counter::COUNT_LATEST);
            sparta::Counter total_time_spent_by_write_requests_=sparta::Counter(getStatisticSet(), "total_time_spent_by_write_requests", "The total time spent by write requests", sparta::Counter::COUNT_LATEST);
            sparta::Counter total_time_spent_in_queue_read_=sparta::Counter(getStatisticSet(), "total_time_spent_in_queue_reads", "The total time spent by read requests in the mc queue", sparta::Counter::COUNT_LATEST);
            sparta::Counter total_time_spent_in_queue_write_=sparta::Counter(getStatisticSet(), "total_time_spent_in_queue_writes", "The total time spent by write requests in the mc queue", sparta::Counter::COUNT_LATEST);

            sparta::Counter average_queue_occupancy_=sparta::Counter(getStatisticSet(), "average_queue_occupancy", "The average number of requests waiting in the memory controller queue", sparta::Counter::COUNT_LATEST);
            sparta::Counter max_queue_occupancy_=sparta::Counter(getStatisticSet(), "max_queue_occupancy", "The maximum number of requests waiting in the memory controller queue", sparta::Counter::COUNT_LATEST);

            uint64_t last_queue_sampling_timestamp=0;

            sparta::StatisticDef avg_latency_read_requests_{
                getStatisticSet(), "avg_latency_read_requests",
                "Average latency per read request",
                getStatisticSet(), "total_time_spent_by_read_requests/read_requests"
            };
            
            sparta::StatisticDef avg_latency_write_requests_{
                getStatisticSet(), "avg_latency_write_requests",
                "Average latency per write request",
                getStatisticSet(), "total_time_spent_by_write_requests/write_requests"
            };
            
            sparta::StatisticDef avg_time_queued_reads{
                getStatisticSet(), "avg_time_queued_reads",
                "Average time in the queue per read request",
                getStatisticSet(), "total_time_spent_in_queue_reads/read_requests"
            };
            
            sparta::StatisticDef avg_time_queued_writes{
                getStatisticSet(), "avg_time_queued_writes",
                "Average time in the queue per write request",
                getStatisticSet(), "total_time_spent_in_queue_writes/write_requests"
            };
            

            /*!
             * \brief Receive a message from the NoC
             * \param mes The message
             */
            void receiveMessage_(const std::shared_ptr<CacheRequest> &mes);

            /*!
             * \brief Send an acknowledgement through the NoC
             * \param req The request that has been completed and will be acknowledged
             */
            void issueAck_(std::shared_ptr<CacheRequest> req);

            /*!
             * \brief Execute a memory controller cycle
             *  This method implements the main operation of the controller and will be
             *  executed every cycle, provided there is work to do
             */
            void controllerCycle_();
    };
}
#endif
