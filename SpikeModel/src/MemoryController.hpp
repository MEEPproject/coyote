
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

#include "LogCapable.hpp"
#include "NoCMessage.hpp"

namespace spike_model
{
    class Core; //Forward declaration    

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


        ////////////////////////////////////////////////////////////////////////////////
        // Type Name/Alias Declaration
        ////////////////////////////////////////////////////////////////////////////////

        void issueAck_(const std::shared_ptr<NoCMessage> & NoCMessage);

    private:
        
            sparta::DataOutPort<std::shared_ptr<NoCMessage>> out_port_noc_
            {&unit_port_set_, "out_noc"};

            sparta::DataInPort<std::shared_ptr<NoCMessage>> in_port_noc_
            {&unit_port_set_, "in_noc"};

            uint64_t latency_;
        
            sparta::Counter count_requests_=sparta::Counter(getStatisticSet(), "requests", "Number of requests", sparta::Counter::COUNT_NORMAL);
    };
}
#endif
