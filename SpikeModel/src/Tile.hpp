
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

#include <DataMappingPolicy.hpp>
#include "RequestManager.hpp"
#include "NoCMessage.hpp"
#include "LogCapable.hpp"

namespace spike_model
{
    class RequestManager; //Forward declaration

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

            void setL2BankInfo(uint64_t size, uint64_t assoc, uint64_t line_size)
            {
                l2_bank_size_kbs=size;
                l2_assoc=assoc;
                l2_line_size=line_size;

                block_offset_bits=(uint8_t)ceil(log2(l2_line_size));
                bank_bits=(uint8_t)ceil(log2(in_ports_l2_reqs_.size()));

                uint64_t total_l2_size=l2_bank_size_kbs*in_ports_l2_reqs_.size()*1024;
                uint64_t num_sets=total_l2_size/(l2_assoc*l2_line_size);
                set_bits=(uint8_t)ceil(log2(num_sets));
                tag_bits=64-(set_bits+block_offset_bits);
                std::cout << "There are " << unsigned(bank_bits) << " bits for banks and " << unsigned(set_bits) << " bits for sets\n";
            }

            void setRequestManager(std::shared_ptr<RequestManager> r)
            {
                request_manager_=r;
            }

            std::shared_ptr<RequestManager> getRequestManager()
            {
                return request_manager_;
            }

            void setId(uint16_t id)
            {
                id_=id;
            }

            void putRequest_(const std::shared_ptr<L2Request> & req, uint64_t lapse);
            void notifyAck_(const std::shared_ptr<L2Request> & req);
            void handleNoCMessage_(const std::shared_ptr<NoCMessage> & mes);

        private:
            uint16_t id_;

            uint16_t num_l2_banks_;
            uint64_t latency_;
 
            std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<L2Request>>>> in_ports_l2_acks_;
            std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<L2Request>>>> in_ports_l2_reqs_;
            sparta::DataInPort<std::shared_ptr<NoCMessage>> in_port_noc_
            {&unit_port_set_, "in_noc"};

            std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<L2Request>>>> out_ports_l2_acks_;
            std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<L2Request>>>> out_ports_l2_reqs_;
            sparta::DataOutPort<std::shared_ptr<NoCMessage>> out_port_noc_
            {&unit_port_set_, "out_noc"};


            uint64_t l2_bank_size_kbs;
            uint64_t l2_assoc;
            uint64_t l2_line_size;

            uint8_t block_offset_bits;
            uint8_t bank_bits;
            uint8_t set_bits; 
            uint8_t tag_bits;
       
            static const uint8_t address_size=8;

            DataMappingPolicy data_mapping_policy_;
            std::shared_ptr<RequestManager> request_manager_;

            void issueMemoryControllerRequest_(const std::shared_ptr<L2Request> & req);
            void issueRemoteL2Request_(const std::shared_ptr<L2Request> & req, uint64_t lapse);
            void issueLocalL2Request_(const std::shared_ptr<L2Request> & req, uint64_t lapse);
            void issueBankAck_(const std::shared_ptr<L2Request> & req);
    };
}
#endif
