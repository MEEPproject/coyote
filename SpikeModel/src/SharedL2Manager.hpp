
#ifndef __SHARED_L2_MANAGER_HH__
#define __SHARED_L2_MANAGER_HH__

#include <memory>
#include "RequestManagerIF.hpp"

namespace spike_model
{
    class SharedL2Manager : public RequestManagerIF
    {
        /**
         * \class spike_model::SharedL2Manager
         *
         * \brief A request manager for L2s that are shared (and partitioned) across all the tiles in the architecture
         */
        public:
            /*!
             * \brief Constructor for the request manager
             * \param tiles The tiles handled my the request manager
             * \param cores_per_tile The number of cores per tile
             * \param address_mapping_policy The address mapping policy that will be used
             */
            SharedL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy);
            
            /*!
             * \brief Constructor for the request manager
             * \param tiles The tiles handled my the request manager
             * \param cores_per_tile The number of cores per tile
             * \param address_mapping_policy The address mapping policy that will be used
             * \param b The cache data mapping policy for tiles
             * \param d The cache data mapping policy for banks within a tile
             */
            SharedL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b, CacheDataMappingPolicy d);

            /*!
             * \brief Forward an L2 request to the memory hierarchy.
             * \param req The request to forward 
             * \param lapse The time for the request to be forwarded with respect to the Sparta clock
             */
            void putRequest(std::shared_ptr<Request> req, uint64_t lapse) override;

        private:
            CacheDataMappingPolicy tile_data_mapping_policy;
    };
}
#endif
