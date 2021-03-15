
#ifndef __PRIVATE_L2_MANAGER_HH__
#define __PRIVATE_L2_MANAGER_HH__

#include <memory>
#include "RequestManagerIF.hpp"

namespace spike_model
{
    class PrivateL2Manager : public RequestManagerIF
    {
        /**
         * \class spike_model::PrivateL2Manager
         *
         * \brief A request manager for tile-private L2s
         */
        public:
            /*!
             * \brief Constructor for the request manager
             * \param tiles The tiles handled my the request manager
             * \param cores_per_tile The number of cores per tile
             * \param address_mapping_policy The address mapping policy that will be used
             */
            PrivateL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy);
            
            /*!
             * \brief Constructor for the request manager
             * \param tiles The tiles handled my the request manager
             * \param cores_per_tile The number of cores per tile
             * \param address_mapping_policy The address mapping policy that will be used
             * \param b The cache data mapping policy for banks within a tile
             */
            PrivateL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b);
            
            /*!
             * \brief Forward an L2 request to the memory hierarchy.
             * \param req The request to forward 
             */
            void putRequest(std::shared_ptr<CacheRequest> req) override;
    };
}
#endif
