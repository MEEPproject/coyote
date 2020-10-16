
#ifndef __REQUEST_MANAGER_HH__
#define __REQUEST_MANAGER_HH__

#include <memory>
#include "ServicedRequests.hpp"
#include "Tile.hpp"

namespace spike_model
{
    class Tile; //Forward declaration    

    class RequestManager
    {
        public:
 
            RequestManager(std::vector<Tile *> tiles, uint16_t cores_per_tile);
            RequestManager(std::vector<Tile *> tiles, uint16_t cores_per_tile, DataMappingPolicy b);

            virtual void putRequest(std::shared_ptr<L2Request> req, uint64_t lapse);
            
            void notifyAck(const std::shared_ptr<L2Request>& req);
        
            void setServicedRequestsStorage(ServicedRequests& s)
            {
                serviced_requests=s;
//                std::cout << s.hasRequest();
            }

            bool hasServicedRequest();

            std::shared_ptr<L2Request> getServicedRequest();

            void setL2TileInfo(uint64_t size, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile);

        protected:
            std::vector<Tile *> tiles_;
            uint16_t cores_per_tile_;
            
            uint8_t block_offset_bits;
            uint8_t tile_bits;
            uint8_t set_bits; 
            uint8_t tag_bits;
            uint8_t bank_bits; 
        
        private:
            ServicedRequests serviced_requests;
            DataMappingPolicy bank_data_mapping_policy;
    };
}
#endif
