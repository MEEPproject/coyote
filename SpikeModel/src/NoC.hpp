
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
#include "LogCapable.hpp"
#include "ServicedRequests.hpp"

namespace spike_model
{
    //class Core; //Forward declaration

    class NoC : public sparta::Unit, public LogCapable
    {
        /*!
         * \class spike_model::NoC
         * \brief NoC models the network on chip interconnecting tiles and memory controllers
         *
         * It currently models a highly idelized crossbar, but more realistic NoCs are work in progress
         */
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
            PARAMETER(uint16_t, num_memory_cpus, 1, "The number of memory CPUs")
            PARAMETER(uint16_t, latency, 1, "The cycles to cross the NoC")
        };

        /*!
         * \brief Constructor for NoC
         * \param node The node that represent the NoC and
         * \param p The NoC parameter set
         */
        NoC(sparta::TreeNode* node, const NoCParameterSet* p);

        ~NoC() {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << std::endl;
        }

        //! name of this resource.
        static const char name[];

        /*! \brief Forward a message sent from a tile to the correct destination
        * \parame mes The meesage to handle
        */
        void handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mes);

        /*! \brief Forward a message sent from a memory CPU to the correct destination
        * \param mes The message
        */
        void handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mes);

    private:

        uint16_t num_tiles_;
        uint16_t num_memory_cpus_;
        uint16_t latency_;

        std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>>> in_ports_tiles_;
        std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>>> out_ports_tiles_;
        std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>>> in_ports_memory_cpus_;
        std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>>> out_ports_memory_cpus_;
    };
}
#endif
