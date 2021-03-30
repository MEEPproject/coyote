
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
#include "BankCommand.hpp"
#include "CommandSchedulerIF.hpp"
#include "MemoryBank.hpp"

namespace spike_model
{
    class MemoryBank; //Forward declaration

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
                PARAMETER(uint64_t, latency, 100, "The latency in the memory controller")
                PARAMETER(uint64_t, num_banks, 8, "The number of banks handled by this memory controller")
                PARAMETER(bool, write_allocate, true, "The write allocation policy")
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

        private:

            sparta::DataOutPort<std::shared_ptr<CacheRequest>> out_port_mcpu_
                {&unit_port_set_, "out_mcpu", 1};

            sparta::DataInPort<std::shared_ptr<CacheRequest>> in_port_mcpu_
                {&unit_port_set_, "in_mcpu"};

            sparta::UniqueEvent<sparta::SchedulingPhase::PostTick> controller_cycle_event_
                {&unit_event_set_, "controller_cycle_", CREATE_SPARTA_HANDLER(MemoryController, controllerCycle_)};

            uint64_t latency_;

            uint64_t num_banks_;

            bool write_allocate_;

            bool idle_=true;

            std::vector<MemoryBank *> banks;

            std::unique_ptr<MemoryAccessSchedulerIF> sched;
            std::unique_ptr<CommandSchedulerIF> ready_commands;

            sparta::Counter count_requests_=sparta::Counter(getStatisticSet(), "requests", "Number of requests", sparta::Counter::COUNT_NORMAL);

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

            /*!
             * \brief Create a command to access the memory using the type of the associated request (READ or WRITE)
             * \param req The request
             * \param bank The bank to access
             * \return A command of the correct type to access the specified bank
             */
            std::shared_ptr<BankCommand> getAccessCommand_(std::shared_ptr<CacheRequest> req, uint64_t bank);

            /*!
             * \brief Create a command to read the line after writing it (if write-allocate is enabled)
             * \param req The request
             * \param bank The bank to access
             * \return A command for the write allocate
             */
            std::shared_ptr<BankCommand> getAllocateCommand_(std::shared_ptr<CacheRequest> req, uint64_t bank);
    };
}
#endif
