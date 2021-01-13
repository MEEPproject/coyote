
#ifndef __PRIVATE_L2_MANAGER_HH__
#define __PRIVATE_L2_MANAGER_HH__

#include <memory>
#include "RequestManagerIF.hpp"

namespace spike_model
{
    class PrivateL2Manager : public RequestManagerIF
    {
        public:
            PrivateL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy);
            PrivateL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b);
            void putRequest(std::shared_ptr<Request> req, uint64_t lapse) override;
    };
}
#endif
