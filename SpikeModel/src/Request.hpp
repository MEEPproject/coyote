#ifndef __REQUEST_HH__
#define __REQUEST_HH__

#include "CacheDataMappingPolicy.hpp"
#include "SpikeEvent.hpp"
#include <iostream>

namespace spike_model
{
    class Request : public SpikeEvent, public std::enable_shared_from_this<Request>
    {
        /**
         * \class spike_model::Request
         *
         * \brief Request contains all the information regarding a request to the L2 cache.
         *
         * Instances of request are the main data structure that is communicated between Spike and the Sparta model
         * and between Sparta Units, either as plain Requests or encapsulated in a NoCMessage.
         *
         */
        
        friend class RequestManagerIF; 
    
        public:
            enum class AccessType
            {
                LOAD,
                STORE,
                FETCH,
                WRITEBACK,
            };
            
            enum class RegType
            {
                INTEGER,
                FLOAT,
                VECTOR,
            };

            //Request(){}
            Request() = delete;
            Request(Request const&) = delete;
            Request& operator=(Request const&) = delete;

            /*!
             * \brief Constructor for Request
             * \param  a The requested address
             * \param  t The type of the request
             * \param  pc The program counter of the requesting instruction
             */
            Request(uint64_t a, AccessType t, uint64_t pc): SpikeEvent(pc), address(a), type(t){}

            /*!
             * \brief Constructor for Request
             * \param a The requested address
             * \param t The type of the request
             * \param pc The program counter of the requesting instruction
             * \param time The timestamp for the request
             * \param c The requesting core
             */
            Request(uint64_t a, AccessType t, uint64_t pc, uint64_t time, uint16_t c): SpikeEvent(pc, time, c), address(a), type(t) {}

            /*!
             * \brief Get the address of the request
             * \return The address of the request
             */
            uint64_t getAddress() const {return address;}

            /*!
             * \brief Get the type of the request
             * \return The type of the request
             */
            AccessType getType() const {return type;}

            /*!
             * \brief Get the id of the destination register for the request
             * \return The id for the destination register
             */
            uint8_t getDestinationRegId() const {return regId;}

            /*!
             * \brief Set the size of the store
             * \param s The size
             */
            void setSize(uint16_t s) {size=s;}

            /*!
             * \brief Get the size of the store
             * \return The size of the store
             */
            uint16_t getSize() const {return size;}


            /*!
             * \brief Set the id and type of the destination register for the request
             * \param r The id of the register
             * \param t The type of the register
             */
            void setDestinationReg(uint8_t r, RegType t) {regId=r; regType=t;}

            /*!
             * \brief Get the type of the destination register for the request
             * \return The type of the register
             */
            RegType getDestinationRegType() const {return regType;}            

            /*!
             * \brief Set the source tile of the request
             * \param source The source tile
             */
            void setSourceTile(uint16_t source)
            {
                source_tile=source;
            }


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

            /* \brief Obtain the address of the first word of the line that contains the requested address
             * \param block_offset_bits The number of bits used to specify the block offset in the addres. 
             *   This is the log2 of the block size
             */
            void calculateLineAddress(uint8_t block_offset_bits)
            {
                line_address=(address >> block_offset_bits) << block_offset_bits;
            }

            /*!
             * \brief Calculate the home tile for the request
             * \param d The data mapping policy to be used in the calculation
             * \param tag_bits The number of bits that are used for the tag
             * \param block_offset_bits The number of bits that are used for the block offset
             * \param set_bits The number of bits that are used for the set
             * \param bank_bits The number of bits that are used for the bank
             */
            uint16_t calculateHome(CacheDataMappingPolicy d, uint8_t tag_bits, uint8_t block_offset_bits, uint8_t set_bits, uint8_t bank_bits);


            /*!
             * \brief Get the home tile for the request
             * \return The home tile
             */
            uint16_t getHomeTile(){return home_tile;}

            /*!
             * \brief Get the source tile for the request
             * \return The source tile
             */
            uint16_t getSourceTile(){return source_tile;}

            /*!
             * \brief Get the bank that will be accessed by the request
             * \return The bank
             */
            uint16_t getCacheBank(){return cache_bank;}

            /*!
             * \brief Get the address of the first word of the line that contains the requested address
             * \return The address of the line
             */
            uint64_t getLineAddress(){return line_address;}
        
            
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
             * \brief Equality operator for instances of Request
             * \param m The request to compare with
             * \return true if both requests are for the same address
             */
            bool operator ==(const Request & m) const
            {
                return m.getAddress()==getAddress();
            }

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(SpikeEventVisitor * v) override
            {
                v->handle(shared_from_this());
            }


        private:
            uint64_t address;

            uint64_t line_address;

            AccessType type;
            uint8_t regId;
            RegType regType;

            uint16_t size;

            uint16_t home_tile;
            uint16_t source_tile;
            uint16_t cache_bank;

            uint64_t memory_controller_;
            uint64_t rank_;
            uint64_t memory_bank_;
            uint64_t row_;
            uint64_t col_;
            
            /*!
             * \brief Set the information of the memory access triggered by the Request
             * \param memory_controller The memory controller that will be accessed
             * \param rank The rank that will be accessed
             * \param bank The bank that will be accessed
             * \param row The row that will be accessed
             * \param col The column that will be accessed
             * \note This method is public but called through friending by instances of RequestManagerIF
             */
            void setMemoryAccessInfo(uint64_t memory_controller, uint64_t rank, uint64_t bank, uint64_t row, uint64_t col);
    };
    
    inline std::ostream & operator<<(std::ostream & Str, Request const & req)
    {
        Str << "0x" << std::hex << req.getAddress() << " @ " << req.getTimestamp();
        return Str;
    }
}
#endif
