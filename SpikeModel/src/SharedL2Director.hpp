
#ifndef __SHARED_L2_DIRECTOR_HH__
#define __SHARED_L2_DIRECTOR_HH__

#include <memory>
#include "AccessDirector.hpp"

namespace spike_model
{
    class SharedL2Director : public AccessDirector
    {
        /**
         * \class spike_model::SharedL2Director
         *
         * \brief A request manager for L2s that are shared (and partitioned) across all the tiles in the architecture
         */
        public:
            
            /*!
             * \brief Constructor for SharedL2Director
             * \param t The tile that contains the SharedL2Director
             * \param address_mapping_policy The mapping policy when accessing main memory
             */
            SharedL2Director(Tile * t, spike_model::AddressMappingPolicy address_mapping_policy) 
                                                    : AccessDirector(t, address_mapping_policy), tile_data_mapping_policy_(CacheDataMappingPolicy::SET_INTERLEAVING){}
            
            /*!
             * \brief Constructor for SharedL2Director
             * \param t The tiles handled my the request manager
             * \param address_mapping_policy The address mapping policy that will be used
             * \param b The cache data mapping policy for banks within a tile
             * \param s The data mapping policy for scratchpad accesses
             * \param d The data mapping policy for tiles
             */
            SharedL2Director(Tile * t, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b, CacheDataMappingPolicy s, CacheDataMappingPolicy d) 
                                                    : AccessDirector(t, address_mapping_policy, b, s), tile_data_mapping_policy_(d){}
            

        private:
            CacheDataMappingPolicy tile_data_mapping_policy_;
            
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
