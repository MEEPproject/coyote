#ifndef __CACHE_REQUEST_HH__
#define __CACHE_REQUEST_HH__

#include "CacheDataMappingPolicy.hpp"
#include "Request.hpp"
#include <iostream>

namespace spike_model
{
    class CacheRequest : public Request, public std::enable_shared_from_this<CacheRequest>
    {
        /**
         * \class spike_model::CacheRequest
         *
         * \brief CacheRequest contains all the information regarding a request to the L2 cache.
         *
         * Instances of request are the main data structure that is communicated between Spike and the Sparta model
         * and between Sparta Units, either as plain CacheRequests or encapsulated in a NoCMessage.
         *
         */
        
        friend class AccessDirector; 
    
        public:
            enum class AccessType
            {
                LOAD,
                STORE,
                FETCH,
                WRITEBACK,
            };
            
            //CacheRequest(){}
            CacheRequest() = delete;
            CacheRequest(Request const&) = delete;
            CacheRequest& operator=(Request const&) = delete;

            /*!
             * \brief Constructor for CacheRequest
             * \param  a The requested address
             * \param  t The type of the request
             * \param  pc The program counter of the requesting instruction
             */
            CacheRequest(uint64_t a, AccessType t, uint64_t pc): Request(a, pc), type(t){}

            /*!
             * \brief Constructor for CacheRequest
             * \param a The requested address
             * \param t The type of the request
             * \param pc The program counter of the requesting instruction
             * \param time The timestamp for the request
             * \param c The requesting core
             */
            CacheRequest(uint64_t a, AccessType t, uint64_t pc, uint64_t time, uint16_t c): Request(a, pc, time, c), type(t) {}

            /*!
             * \brief Get the type of the request
             * \return The type of the request
             */
            AccessType getType() const {return type;}


            /*!
             * \bref Set the home tile of the request
             * \param home The home tile
             */
            void setHomeTile(uint16_t home)
            {
                home_tile=home;
            }


            /*!
             * \brief Set the cache bank that will be accessed by the request
             * \param b The cache bank
             */
            void setCacheBank(uint16_t b)
            {
                cache_bank=b;
            }



            /*!
             * \brief Get the home tile for the request
             * \return The home tile
             */
            uint16_t getHomeTile(){return home_tile;}

            /*!
             * \brief Get the bank that will be accessed by the request
             * \return The bank
             */
            uint16_t getCacheBank(){return cache_bank;}

            
            /*!
             * \brief Get the memory controller that will be accessed
             * \return The address of the line
             */
            uint64_t getMemoryController();
            
            /*!
             * \brief Get the rank taht will be accessed
             * \return The address of the line
             */
            uint64_t getRank();
            
            /*!
             * \brief Get the bank that will be accessed
             * \return The address of the line
             */
            uint64_t getMemoryBank();
            
            /*!
             * \brief Get the row that will be accessed
             * \return The address of the line
             */
            uint64_t getRow();
            
            /*!
             * \brief Get the column that will be accessed
             * \return The address of the line
             */
            uint64_t getCol();

            /*
             * \brief Equality operator for instances of CacheRequest
             * \param m The request to compare with
             * \return true if both requests are for the same address
             */
            bool operator ==(const CacheRequest & m) const
            {
                return m.getAddress()==getAddress();
            }

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override
            {
                v->handle(shared_from_this());
            }


        private:
            AccessType type;

            uint16_t home_tile;
            uint16_t cache_bank;

            uint64_t memory_controller_;
            uint64_t rank_;
            uint64_t memory_bank_;
            uint64_t row_;
            uint64_t col_;
            
            /*!
             * \brief Set the information of the memory access triggered by the CacheRequest
             * \param memory_controller The memory controller that will be accessed
             * \param rank The rank that will be accessed
             * \param bank The bank that will be accessed
             * \param row The row that will be accessed
             * \param col The column that will be accessed
             * \note This method is public but called through friending by instances of CacheEventManager
             */
            void setMemoryAccessInfo(uint64_t memory_controller, uint64_t rank, uint64_t bank, uint64_t row, uint64_t col);
    };
    
    inline std::ostream & operator<<(std::ostream & Str, CacheRequest const & req)
    {
        Str << "0x" << std::hex << req.getAddress() << " @ " << req.getTimestamp();
        return Str;
    }
}
#endif
