
#ifndef __REQUEST_MANAGER_HH__
#define __REQUEST_MANAGER_HH__

#include <memory>
#include "ServicedRequests.hpp"
#include "Tile.hpp"
#include "NoCMessage.hpp"
#include "AddressMappingPolicy.hpp"

namespace spike_model
{
    class Tile; //Forward declaration    

    class RequestManagerIF
    {
        public:
 
            RequestManagerIF(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy);
            RequestManagerIF(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b);

            virtual void putRequest(std::shared_ptr<Request> req, uint64_t lapse);
            
            void notifyAck(const std::shared_ptr<Request>& req);
        
            void setServicedRequestsStorage(ServicedRequests& s)
            {
                serviced_requests_=s;
//                std::cout << s.hasRequest();
            }

            bool hasServicedRequest();

            std::shared_ptr<Request> getServicedRequest();

            void setMemoryInfo(uint64_t l2_tile_size, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile, uint64_t num_mcs, uint64_t num_banks_per_mc, uint64_t num_rows_per_bank, uint64_t num_cols_per_bank);

            std::shared_ptr<NoCMessage> getRemoteL2RequestMessage(std::shared_ptr<Request> req);
            std::shared_ptr<NoCMessage> getMemoryRequestMessage(std::shared_ptr<Request> req);
            std::shared_ptr<NoCMessage> getMemoryReplyMessage(std::shared_ptr<Request> req);
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

            static const uint8_t address_size=8;
    };
}
#endif
