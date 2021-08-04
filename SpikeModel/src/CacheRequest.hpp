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
        friend class MemoryController; 
    
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
            CacheRequest(uint64_t a, AccessType t, uint64_t pc): Request(a, pc), type(t) {
                memory_ack=false;
                parentinstruction_ID = 0; // 0 is not used by the memory tile
            }

            /*!
             * \brief Constructor for CacheRequest
             * \param a The requested address
             * \param t The type of the request
             * \param pc The program counter of the requesting instruction
             * \param time The timestamp for the request
             * \param c The requesting core
             */
            CacheRequest(uint64_t a, AccessType t, uint64_t pc, uint64_t time, uint16_t c): Request(a, pc, time, c), type(t) {
                memory_ack=false;
                parentinstruction_ID = 0; // 0 is not used by the memory tile
            }

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
             * \brief Get the home tile for the request
             * \return The home tile
             */
            uint16_t getHomeTile(){return home_tile;}
            
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

            bool memoryAck()
            {
                return memory_ack;
            }

            void setMemoryAck(bool ack)
            {
                memory_ack = ack;
            }

            uint32_t getParentInstruction_ID()
            {
               return  parentinstruction_ID ;
            }

            void setParentInstruction_ID(uint32_t instr_id)
            {
                parentinstruction_ID = instr_id;
            }

        private:
            AccessType type;

            uint16_t home_tile;

            uint32_t parentinstruction_ID; // id of the mcpu instruction it was generated from

            uint64_t memory_controller_;
            uint64_t rank_;
            uint64_t memory_bank_;
            uint64_t row_;
            uint64_t col_;
            bool memory_ack;
            
            /*!
             * \brief Set the bank information of the memory access triggered by the CacheRequest
             * \param rank The rank that will be accessed
             * \param bank The bank that will be accessed
             * \param row The row that will be accessed
             * \param col The column that will be accessed
             * \note This method is private but called through friending by instances of MemoryController
             */
            void setBankInfo(uint64_t rank, uint64_t bank, uint64_t row, uint64_t col);
            
            /*!
             * \brief Set the information of the memory access triggered by the CacheRequest
             * \param memory_controller The memory controller that will be accessed
             * \note This method is private but called through friending by instances of AccessDirector
             */
            void setMemoryController(uint64_t memory_controller);


    };
    
    inline std::ostream& operator<<(std::ostream &str, CacheRequest const &req)
    {
        str << "0x" << std::hex << req.getAddress() << " @ " << req.getTimestamp() << ", coreID: " << req.getCoreId();
        return str;
    }
}
#endif
