
#ifndef __REQUEST_MANAGER_HH__
#define __REQUEST_MANAGER_HH__

#include <memory>
#include "ServicedRequests.hpp"
#include "Tile.hpp"
#include "NoCMessage.hpp"
#include "AddressMappingPolicy.hpp"

class SpikeModel; //Forward declaration

namespace spike_model
{
    class Tile; //Forward declaration    

    class RequestManagerIF
    {
        /**
         * \class spike_model::RequestManagerIF
         *
         * \brief RequestManagerIF is the main interface between Spike and Sparta.
         *
         * Instances of SimulationOrchestrator use an instance of RequestManagerIF to forward a Request 
         * to Sparta and check for its completion. The RequestManagerIF also holds information regarding
         * the memory hiwerarchy of the modelled architecture, to update the data of requests.
         *
         */

        friend class ::SpikeModel;

        public:
 
            /*!
             * \brief Constructor for the request manager
             * \param tiles The tiles handled my the request manager
             * \param cores_per_tile The number of cores per tile
             * \param address_mapping_policy The address mapping policy that will be used
             */
            RequestManagerIF(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy);
            
            /*!
             * \brief Constructor for the request manager
             * \param tiles The tiles handled my the request manager
             * \param cores_per_tile The number of cores per tile
             * \param address_mapping_policy The address mapping policy that will be used
             * \param b The cache data mapping policy that will be used
             */
            RequestManagerIF(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b);

            /*!
             * \brief Forward an L2 request to the memory hierarchy.
             * \param req The request to forward 
             * \param lapse The time for the request to be forwarded with respect to the Sparta clock
             */
            virtual void putRequest(std::shared_ptr<Request> req, uint64_t lapse);
            
            /*!
             * \brief Notify the completion of request
             * \param req The request that has been completed
             */
            void notifyAck(const std::shared_ptr<Request>& req);
        

            /*!
             * \brief Query for the availability of serviced requests
             * \return true if there is any serviced request
             */
            bool hasServicedRequest();

            /*!
             * \brief Get a serviced request
             * \return A request that has been serviced
             */
            std::shared_ptr<Request> getServicedRequest();


            /*!
             * \brief Get a NoCMessage representing a remote L2 Request
             * \param req The request assocaited to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getRemoteL2RequestMessage(std::shared_ptr<Request> req);
            /*!
             * \brief Get a NoCMessage representing a memory Request
             * \param req The request assocaited to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getMemoryRequestMessage(std::shared_ptr<Request> req);
            /*!
             * \brief Get a NoCMessage representing a reply
             * \param req The request assocaited to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getMemoryReplyMessage(std::shared_ptr<Request> req);
            /*!
             * \brief Get a NoCMessage representing a data forward
             * \param req The request assocaited to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getDataForwardMessage(std::shared_ptr<Request> req);

        protected:
            std::vector<Tile *> tiles_;
            uint16_t cores_per_tile_;
           
            uint64_t line_size;

            uint8_t block_offset_bits;
            uint8_t tile_bits;
            uint8_t set_bits; 
            uint8_t tag_bits;
            uint8_t bank_bits;

        
        private:
            ServicedRequests serviced_requests_;
            CacheDataMappingPolicy bank_data_mapping_policy_;
            AddressMappingPolicy address_mapping_policy_;
            
            uint64_t mc_shift; //One channel per memory controller
            uint64_t rank_shift;
            uint64_t bank_shift;
            uint64_t row_shift;
            uint64_t col_shift;
            
            uint64_t mc_mask;
            uint64_t rank_mask;
            uint64_t bank_mask;
            uint64_t row_mask;
            uint64_t col_mask;

            const uint8_t address_size=8;
            
            /*!
             * \brief Set the storage for the requests that have been acknowledged 
             * \param s The storage for acknowledged requests
             * \note This method is called through friending by SpikeModel
             */
            void setServicedRequestsStorage(ServicedRequests& s)
            {
                serviced_requests_=s;
//                std::cout << s.hasRequest();
            }
            
            /*!
             * \brief Set the information on the memory hierarchy
             * \param l2_tile_size The total size of the L2 that belongs to each Tile
             * \param assoc The associativity
             * \param line_size The line size
             * \param banks_per_tile The number of banks per Tile
             * \param num_mcs The number of memory controllers
             * \param num_banks_per_mc The number of banks per memory controller
             * \param num_rows_per_bank The number of rows per memory bank
             * \param num_cols_per_bank The number of columns per memory bank
             * \note This method is called through friending by SpikeModel
             */
            void setMemoryInfo(uint64_t l2_tile_size, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile, 
                                uint64_t num_mcs, uint64_t num_banks_per_mc, uint64_t num_rows_per_bank, uint64_t num_cols_per_bank);
    };
}
#endif
