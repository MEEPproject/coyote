
#ifndef __ACCESS_DIRECTOR_HH__
#define __ACCESS_DIRECTOR_HH__

#include "AddressMappingPolicy.hpp"
#include "CacheDataMappingPolicy.hpp"
#include "EventVisitor.hpp"

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
             * \param address_mapping_policy The mapping policy when accessing main memory
             */
            AccessDirector(Tile * t, spike_model::AddressMappingPolicy address_mapping_policy) 
                                                    : tile(t), address_mapping_policy_(address_mapping_policy), bank_data_mapping_policy_(CacheDataMappingPolicy::PAGE_TO_BANK){}
            
            /*!
             * \brief Constructor for AccessDirector
             * \param t The tile that contains the AccessDirector
             * \param address_mapping_policy The mapping policy when accessing main memory
             */
            AccessDirector(Tile * t, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b) 
                                                    : tile(t), address_mapping_policy_(address_mapping_policy), bank_data_mapping_policy_(b){}

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
             * \brief Set the information on the memory hierarchy
             * \param l2_tile_size The total size of the L2 that belongs to each Tile
             * \param assoc The associativity
             * \param line_size The line size
             * \param banks_per_tile The number of banks per Tile
             * \param the number of tiles in the system
             * \param num_mcs The number of memory controllers
             * \param num_banks_per_mc The number of banks per memory controller
             * \param num_rows_per_bank The number of rows per memory bank
             * \param num_cols_per_bank The number of columns per memory bank
             */
            void setMemoryInfo(uint64_t l2_tile_size, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile,  uint16_t num_tiles, 
                                uint64_t num_mcs, uint64_t num_banks_per_mc, uint64_t num_rows_per_bank, uint64_t num_cols_per_bank);
            
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

        protected:
            uint64_t line_size;

            uint8_t tag_bits;
            uint8_t block_offset_bits;
            uint8_t tile_bits;
            uint8_t set_bits;
            uint8_t bank_bits;
            

        private:
            Tile * tile;

            AddressMappingPolicy address_mapping_policy_;
            
            uint64_t mc_shift; //One channel per memory controller
            uint64_t rank_shift;
            uint64_t bank_shift;
            uint64_t row_shift;
            uint64_t col_shift;
            
            uint64_t mc_mask;
            uint64_t rank_mask;
            uint64_t bank_mask;
            uint64_t row_mask;
            uint64_t col_mask;

            const uint8_t address_size=8; //In bytes
             
            /*!              
            * \brief Calculate the home tile and bank for a request
            * \param r A Request
            * \return The home tile
            */
            virtual uint16_t calculateHome(std::shared_ptr<spike_model::CacheRequest> r)=0;
            
            /*!              
            * \brief Calculate the bank for a request
            * \param r A Request
            * \return The bank to access
            */
            virtual uint16_t calculateBank(std::shared_ptr<spike_model::CacheRequest> r)=0;

        protected:
            CacheDataMappingPolicy bank_data_mapping_policy_;
    };

}
#endif
