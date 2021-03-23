
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
#include "NoCMessage.hpp"
#include "MemoryAccessSchedulerIF.hpp"
#include "EventManager.hpp"
#include "BankCommand.hpp"
#include "CommandSchedulerIF.hpp"
#include "MemoryBank.hpp"
#include "Event.hpp"
#include "MCPURequest.hpp"

namespace spike_model
{
    class MemoryBank; //Forward declaration

    class MemoryController : public sparta::Unit, public LogCapable, public spike_model::EventVisitor
    {
        using spike_model::EventVisitor::handle; //This prevents the compiler from warning on overloading
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
                PARAMETER(uint64_t, line_size, 128, "The cache line size")
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
            
            /*!
             * \brief Set the request manager
             * \param r The request manager
             */
            void setRequestManager(std::shared_ptr<EventManager> r);

            virtual void handle(std::shared_ptr<spike_model::CacheRequest> r) override;
            virtual void handle(std::shared_ptr<spike_model::MCPURequest> r) override;

        private:
            
            sparta::DataOutPort<std::shared_ptr<NoCMessage>> out_port_noc_
                {&unit_port_set_, "out_noc"};

            sparta::DataInPort<std::shared_ptr<NoCMessage>> in_port_noc_
                {&unit_port_set_, "in_noc"};

            sparta::UniqueEvent<> controller_cycle_event_ 
                {&unit_event_set_, "controller_cycle_", CREATE_SPARTA_HANDLER(MemoryController, controllerCycle_)};

            uint64_t latency_;
       
            uint64_t line_size_;

            uint64_t num_banks_;

            bool write_allocate_;

            bool idle_=true;

            std::vector<MemoryBank *> banks;

            std::unique_ptr<MemoryAccessSchedulerIF> sched;
            std::unique_ptr<CommandSchedulerIF> ready_commands;
    
            std::shared_ptr<EventManager> request_manager_;

            sparta::Counter count_requests_=sparta::Counter(getStatisticSet(), "requests", "Number of requests", sparta::Counter::COUNT_NORMAL);
    
            /*!
             * \brief Receive a message from the NoC
             * \param mes The message
             */
            void receiveMessage_(const std::shared_ptr<NoCMessage> & mes);
            
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
