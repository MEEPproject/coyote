
#ifndef __PRIVATE_L2_MANAGER_HH__
#define __PRIVATE_L2_MANAGER_HH__

#include <memory>
#include "RequestManager.hpp"

namespace spike_model
{
    class PrivateL2Manager : public RequestManager
    {
        public:
            PrivateL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile);
            PrivateL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, DataMappingPolicy b);
            void putRequest(std::shared_ptr<L2Request> req, uint64_t lapse) override;
    };
}
#endif
