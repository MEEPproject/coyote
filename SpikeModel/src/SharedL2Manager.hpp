
#ifndef __SHARED_L2_MANAGER_HH__
#define __SHARED_L2_MANAGER_HH__

#include <memory>
#include "RequestManagerIF.hpp"

namespace spike_model
{
    class SharedL2Manager : public RequestManagerIF
    {
        public:
            SharedL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy);
            SharedL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b, CacheDataMappingPolicy d);

            void putRequest(std::shared_ptr<Request> req, uint64_t lapse) override;

        private:
            CacheDataMappingPolicy tile_data_mapping_policy;
    };
}
#endif
