
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
             */
            SharedL2Director(Tile * t) : AccessDirector(t), tile_data_mapping_policy_(CacheDataMappingPolicy::SET_INTERLEAVING){}
            
            /*!
             * \brief Constructor for SharedL2Director
             * \param t The tiles handled my the request manager
             * \param b The cache data mapping policy for banks within a tile
             * \param s The data mapping policy for scratchpad accesses
             * \param d The data mapping policy for tiles
             */
            SharedL2Director(Tile * t, CacheDataMappingPolicy b, VRegMappingPolicy s, CacheDataMappingPolicy d) 
                                                    : AccessDirector(t, b, s), tile_data_mapping_policy_(d){}
            

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
