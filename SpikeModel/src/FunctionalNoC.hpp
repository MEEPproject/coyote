
#ifndef __FUNCTIONAL_NOC_H__
#define __FUNCTIONAL_NOC_H__

#include "NoC.hpp"

namespace spike_model
{
    class FunctionalNoC : public NoC
    {
        /*!
         * \class spike_model::FunctionalNoC
         * \brief Functional model of the NoC with a predefined latency of averaged_packet_latency
         * 
         * It models an idealized network in which each transaction takes a predefined number of cycles
         */
    public:
        /*!
         * \class NoCParameterSet
         * \brief Parameters for functional NoC
         */
        class FunctionalNoCParameterSet : public NoCParameterSet
        {
        public:
            //! Constructor for FunctionalNoCParameterSet
            FunctionalNoCParameterSet(sparta::TreeNode* node) :
                NoCParameterSet(node)
            {
            }
            PARAMETER(uint16_t, packet_latency, 30, "The average latency for each packet")
        };

        /*!
         * \brief Constructor for FunctionalNoC
         * \param node The node that represent the NoC and
         * \param params The FunctionalNoC parameter set
         */
        FunctionalNoC(sparta::TreeNode* node, const FunctionalNoCParameterSet* params);

        ~FunctionalNoC() {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << std::endl;
        }

        /*! \brief Forwards a message from TILE to the actual destination using a predefined latency
         *   \param mess The message to handle
         */
        void handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess) override;
        
        /*! \brief Forwards a message from a memory controller to the correct destination using a predefined latency
         *   \param mess The message
         */
        void handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess) override;

    private:

        uint16_t    packet_latency_;   //! The latency imposed to each packet

    };

} // spike_model

#endif // __FUNCTIONAL_NOC_H__
