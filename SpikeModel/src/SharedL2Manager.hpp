
#ifndef __SHARED_L2_MANAGER_HH__
#define __SHARED_L2_MANAGER_HH__

#include <memory>
#include "RequestManager.hpp"

namespace spike_model
{
    class SharedL2Manager : public RequestManager
    {
        public:
            SharedL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile);
            SharedL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, DataMappingPolicy b, DataMappingPolicy d);

            void putRequest(std::shared_ptr<L2Request> req, uint64_t lapse) override;

        private:
            DataMappingPolicy tile_data_mapping_policy;
    };
}
#endif
