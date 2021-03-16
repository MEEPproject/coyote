
#ifndef __PRIVATE_L2_DIRECTOR_HH__
#define __PRIVATE_L2_DIRECTOR_HH__

#include <memory>
#include "AccessDirector.hpp"

namespace spike_model
{
    class PrivateL2Director : public AccessDirector
    {
        /**
         * \class spike_model::PrivateL2Director
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
            PrivateL2Director(Tile * t, spike_model::AddressMappingPolicy address_mapping_policy) : AccessDirector(t, address_mapping_policy) {}
            
            /*!
             * \brief Constructor for the request manager
             * \param tiles The tiles handled my the request manager
             * \param cores_per_tile The number of cores per tile
             * \param address_mapping_policy The address mapping policy that will be used
             * \param b The cache data mapping policy for banks within a tile
             */
            PrivateL2Director(Tile * t, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b) : AccessDirector(t, address_mapping_policy, b) {}
            

        private:
            /*!              
            * \brief Calculate the home tile for a request
            * \param r A Request
            * \return The home tile
            */
            uint16_t calculateHome(std::shared_ptr<spike_model::CacheRequest> r) override;
            
            /*!              
            * \brief Calculate the bank for a request
            * \param r A Request
            * \return The bank to access
            */
            uint16_t calculateBank(std::shared_ptr<spike_model::CacheRequest> r);
    };
}
#endif
