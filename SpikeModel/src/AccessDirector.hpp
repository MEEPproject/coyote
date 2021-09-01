
#ifndef __ACCESS_DIRECTOR_HH__
#define __ACCESS_DIRECTOR_HH__

#include "AddressMappingPolicy.hpp"
#include "CacheDataMappingPolicy.hpp"
#include "EventVisitor.hpp"

#include <map>

namespace spike_model
{
    class Tile; //Forward declaration
    class NoCMessage;

    class AccessDirector : public spike_model::EventVisitor
    {
        using spike_model::EventVisitor::handle; //This prevents the compiler from warning on overloading 

        /*!
         * \class AccessDirector 
         * \brief The component in charge of forwarding requests to the correct element of the memory hierarchy
         */
        public:
            
            /*!
             * \brief Constructor for AccessDirector
             * \param t The tile that contains the AccessDirector
             */
            AccessDirector(Tile * t) 
                                                    : tile(t), bank_data_mapping_policy_(CacheDataMappingPolicy::PAGE_TO_BANK), 
                                                      scratchpad_data_mapping_policy_(CacheDataMappingPolicy::SET_INTERLEAVING), pending_scratchpad_management_ops(){}
            
            /*!
             * \brief Constructor for AccessDirector
             * \param t The tile that contains the AccessDirector
             * \param b The data mapping policy for cache access
             * \param s The data mapping policy for scratchpad accesses
             */
            AccessDirector(Tile * t, CacheDataMappingPolicy b, CacheDataMappingPolicy s) 
                                                    : tile(t), bank_data_mapping_policy_(b), 
                                                      scratchpad_data_mapping_policy_(s), pending_scratchpad_management_ops(){}

            /*!
             * \brief Forwards an access to the director
             * \param access The Request to forward
             */
            virtual void putAccess(std::shared_ptr<Request> access);
         
            /*!
             * \brief Handles a cache request
             * \param r The event to handle
             */
            void handle(std::shared_ptr<spike_model::CacheRequest> r) override;
            
            /*!
             * \brief Handles a scratchpad request request
             * \param r The event to handle
             */
            void handle(std::shared_ptr<spike_model::ScratchpadRequest> r) override;
            
            /*!
             * \brief Set the information on the memory hierarchy
             * \param l2_tile_size The total size of the L2 that belongs to each Tile
             * \param assoc The associativity
             * \param line_size The line size
             * \param banks_per_tile The number of banks per Tile
             * \param the number of tiles in the system
             * \param num_mcs The number of memory controllers
             * \param memory_controller_shift The number of bits to shift to get the memory controller that handles an address
             * \param memory_controller_mask The mask for the AND to extract the memory controller id
             */
            void setMemoryInfo(uint64_t l2_tile_size, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile,  uint16_t num_tiles, 
                                uint64_t num_mcs, uint64_t memory_controller_shift, uint64_t memory_controller_mask);
            
            /*!
             * \brief Get a NoCMessage representing a remote L2 Request
             * \param req The request assocaited to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getRemoteL2RequestMessage(std::shared_ptr<CacheRequest> req);
            /*!
             * \brief Get a NoCMessage representing a memory Request
             * \param req The request assocaited to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getMemoryRequestMessage(std::shared_ptr<CacheRequest> req);
            
            /*!
             * \brief Get a NoCMessage representing a data forward
             * \param req The request assocaited to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getDataForwardMessage(std::shared_ptr<CacheRequest> req);

            /*!
             * \brief Get a NoCMessage representing a data forward
             * \param req The request assocaited to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getScratchpadAckMessage(std::shared_ptr<ScratchpadRequest> req);

        protected:
            uint64_t line_size;

            uint8_t tag_bits;
            uint8_t block_offset_bits;
            uint8_t tile_bits;
            uint8_t set_bits;
            uint8_t bank_bits;
            

        private:
            Tile * tile;

            uint64_t num_ways;
            uint64_t way_size;

            uint64_t scratchpad_ways=0;
            uint64_t scratchpad_available_size=0;
            
            uint64_t mc_shift; //One channel per memory controller
            uint64_t mc_mask;

            const uint8_t address_size=8; //In bytes
             
            /*!              
            * \brief Calculate the home tile and bank for a request
            * \param r A Request
            * \return The home tile
            */
            virtual uint16_t calculateHome(std::shared_ptr<spike_model::CacheRequest> r)=0;
            
            /*!              
            * \brief Calculate the bank for a cache request
            * \param r A CacheRequest
            * \return The bank to access
            */
            virtual uint16_t calculateBank(std::shared_ptr<spike_model::CacheRequest> r)=0;
    
            /*!              
            * \brief Calculate the bank for a scratchpad request
            * \param r A ScratchpadRequest
            * \return The bank to access
            */
            uint16_t calculateBank(std::shared_ptr<spike_model::ScratchpadRequest> r);
            
        protected:
            CacheDataMappingPolicy bank_data_mapping_policy_;
            CacheDataMappingPolicy scratchpad_data_mapping_policy_;

        private: 
            std::map<std::shared_ptr<spike_model::ScratchpadRequest>, uint64_t> pending_scratchpad_management_ops;

    };

}
#endif
