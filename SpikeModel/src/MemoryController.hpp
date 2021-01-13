
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
#include "RequestManagerIF.hpp"
#include "BankCommand.hpp"
#include "CommandSchedulerIF.hpp"
#include "MemoryBank.hpp"

namespace spike_model
{
    class MemoryBank; //Forward declaration

    class MemoryController : public sparta::Unit, public LogCapable
    {
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
            };

            /*!
             * \brief Constructor for MemoryController
             * \note  node parameter is the node that represent the MemoryController and
             *        p is the MemoryController parameter set
             */
            MemoryController(sparta::TreeNode* node, const MemoryControllerParameterSet* p);

            ~MemoryController() {
                debug_logger_ << getContainer()->getLocation()
                              << ": "
                              << std::endl;
            }

            //! name of this resource.
            static const char name[];

            void addBank_(MemoryBank * bank);
            void notifyCompletion_(std::shared_ptr<BankCommand> c);
            
            /*!
             * \brief Sets the request manager for the tile
             */
            void setRequestManager(std::shared_ptr<RequestManagerIF> r);

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

            bool idle_=true;

            std::vector<MemoryBank *> banks;

            std::unique_ptr<MemoryAccessSchedulerIF> sched;
            std::unique_ptr<CommandSchedulerIF> ready_commands;
    
            std::shared_ptr<RequestManagerIF> request_manager_;

            sparta::Counter count_requests_=sparta::Counter(getStatisticSet(), "requests", "Number of requests", sparta::Counter::COUNT_NORMAL);
    
            void receiveMessage_(const std::shared_ptr<NoCMessage> & mes);
            
            void issueAck_(std::shared_ptr<Request> req);
    
            void controllerCycle_();
    
            std::shared_ptr<BankCommand> getAccessCommand_(std::shared_ptr<Request> req, uint64_t bank);
    };
}
#endif
