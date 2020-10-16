
#ifndef __NoC_H__
#define __NoC_H__

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

#include "NoCMessage.hpp"
#include "Core.hpp"
#include "LogCapable.hpp"
#include "ServicedRequests.hpp"

namespace spike_model
{
    class Core; //Forward declaration    

    class NoC : public sparta::Unit, public LogCapable
    {
    public:
        /*!
         * \class NoCParameterSet
         * \brief Parameters for NoC model
         */
        class NoCParameterSet : public sparta::ParameterSet
        {
        public:
            //! Constructor for NoCParameterSet
            NoCParameterSet(sparta::TreeNode* n):
                sparta::ParameterSet(n)
            {
            }
            PARAMETER(uint16_t, num_tiles, 1, "The number of tiles")
            PARAMETER(uint16_t, num_memory_controllers, 1, "The number of memory controllers")
            PARAMETER(uint16_t, latency, 1, "The cycles to cross the NoC")
        };

        /*!
         * \brief Constructor for NoC
         * \note  node parameter is the node that represent the NoC and
         *        p is the NoC parameter set
         */
        NoC(sparta::TreeNode* node, const NoCParameterSet* p);

        ~NoC() {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << std::endl;
        }

        //! name of this resource.
        static const char name[];


        ////////////////////////////////////////////////////////////////////////////////
        // Type Name/Alias Declaration
        ////////////////////////////////////////////////////////////////////////////////

        void handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mes);
        void issueAck_(const std::shared_ptr<NoCMessage> & mes);

    private:

        uint16_t num_tiles_;
        uint16_t num_memory_controllers_;
        uint16_t latency_;
 
        std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>>> in_ports_tiles_;
        std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>>> out_ports_tiles_;
        std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>>> in_ports_memory_controllers_;
        std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>>> out_ports_memory_controllers_;


        uint16_t getDestination(std::shared_ptr<NoCMessage> req);

        uint64_t latency=1;
    };
}
#endif
