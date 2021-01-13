
#ifndef __TILE_HH__
#define __TILE_HH__

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

#include "RequestManagerIF.hpp"
#include "NoCMessage.hpp"
#include "LogCapable.hpp"

namespace spike_model
{
    class RequestManagerIF; //Forward declaration
    class NoCMessage; //Forward declaration

    class Tile : public sparta::Unit, public LogCapable
    {
        public:
 
            /*!
             * \class TileParameterSet
             * \brief Parameters for Tile model
             */
            class TileParameterSet : public sparta::ParameterSet
            {
            public:
                //! Constructor for TileParameterSet
                TileParameterSet(sparta::TreeNode* n):
                    sparta::ParameterSet(n)
                {
                }
                PARAMETER(uint16_t, num_l2_banks, 1, "The number of cache banks in the tile")
                PARAMETER(uint64_t, latency, 1, "The number of cycles to get to a local cache bank")
            };

            /*!
             * \brief Constructor for Tile
             * \note  node parameter is the node that represent the Tile and
             *        p is the Tile parameter set
             */
            Tile(sparta::TreeNode* node, const TileParameterSet* p);

            ~Tile() {
                debug_logger_ << getContainer()->getLocation()
                              << ": "
                              << std::endl;
            }

            //! name of this resource.
            static const char name[];

            /*!
             * \brief Sets the information on the l2 cache banks associated to this tile
             */
            void setL2BankInfo(uint64_t size, uint64_t assoc, uint64_t line_size);

            /*!
             * \brief Sets the request manager for the tile
             */
            void setRequestManager(std::shared_ptr<RequestManagerIF> r);

            /*!
             * \brief Sets the id for the tile
             */
            void setId(uint16_t id);

            /*!
             * \brief Enqueues an L2 request to the tile
             * \note lapse is the number of cycles that the request needs to be delayed
             */
            void putRequest_(const std::shared_ptr<Request> & req, uint64_t lapse);
            
            /*!
             * \brief Notifies the completion of the service for an L2 request
             */
            void notifyAck_(const std::shared_ptr<Request> & req);
            

        private:
            uint16_t id_;

            uint16_t num_l2_banks_;
            uint64_t latency_;
 
            std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<Request>>>> in_ports_l2_acks_;
            std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<Request>>>> in_ports_l2_reqs_;
            sparta::DataInPort<std::shared_ptr<NoCMessage>> in_port_noc_
            {&unit_port_set_, "in_noc"};

            std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<Request>>>> out_ports_l2_acks_;
            std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<Request>>>> out_ports_l2_reqs_;
            sparta::DataOutPort<std::shared_ptr<NoCMessage>> out_port_noc_
            {&unit_port_set_, "out_noc"};


            uint64_t l2_bank_size_kbs;
            uint64_t l2_assoc;
            uint64_t l2_line_size;

            uint8_t block_offset_bits;
            uint8_t bank_bits;
            uint8_t set_bits; 
            uint8_t tag_bits;
       

            std::shared_ptr<RequestManagerIF> request_manager_;

            /*!
             * \brief Sends a request to a memory controller
             */
            void issueMemoryControllerRequest_(const std::shared_ptr<Request> & req);
            
            /*!
             * \brief Sends an L2 request to a bank in a different tile
             */
            void issueRemoteRequest_(const std::shared_ptr<Request> & req, uint64_t lapse);
            
            /*!
             * \brief Sends an L2 request to a bank in the current tile
             */
            void issueLocalRequest_(const std::shared_ptr<Request> & req, uint64_t lapse);
            
            /*!
             * \brief Sends an ack to a cache bank of the current tile (a prior remote L2 request or memory request has been serviced)
             */
            void issueBankAck_(const std::shared_ptr<Request> & req);
            
            /*!
             * \brief Handles a message from the NoC
             */
            void handleNoCMessage_(const std::shared_ptr<NoCMessage> & mes);
    };
}
#endif
