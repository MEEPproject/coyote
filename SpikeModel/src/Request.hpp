#ifndef __L2_REQUEST_HH__
#define __L2_REQUEST_HH__

#include "CacheDataMappingPolicy.hpp"
#include <iostream>

namespace spike_model
{
    class Request
    {
        public:
            enum class AccessType
            {
                LOAD,
                STORE,
                FETCH,
                WRITEBACK,
                FINISH 
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
             * \note  a is the requested address
             *        t is the type of the request
             *        pc is the program counter of the requesting instruction
             */
            Request(uint64_t a, AccessType t, uint64_t pc): address(a), type(t), pc(pc){}

            /*!
             * \brief Constructor for Request
             * \note  a is the requested address
             *        t is the type of the request
             *        pc is the program counter of the requesting instruction
             *        time is the timestamp for the request
             *        c is the requesting core
             */
            Request(uint64_t a, AccessType t, uint64_t pc, uint64_t time, uint16_t c): address(a), type(t), pc(pc), timestamp(time), coreId(c){}

            /*!
             * \brief Returns the address of the request
             */
            uint64_t getAddress() const {return address;}

            /*!
             * \brief Returns the type of the request
             */
            AccessType getType() const {return type;}

            /*!
             * \brief Returns the timestamp of the request
             */
            uint64_t getTimestamp() const {return timestamp;}

            /*!
             * \brief Return the requesting core
             */
            uint64_t getCoreId() const {return coreId;}

            /*!
             * \brief Set the timestamp of the request
             */
            void setTimestamp(uint64_t t) {timestamp=t;}

            /*!
             * \brief Sets the requesting core
             */
            void setCoreId(uint16_t c) {coreId=c;}

            /*!
             * \brief Returns the id of the destination register for the request
             */
            uint8_t getDestinationRegId() const {return regId;}

            /*!
             * \brief Sets the id and type of the destination register for the request
             */
            void setDestinationReg(uint8_t r, RegType t) {regId=r; regType=t;}

            /*!
             * \brief Returns the type of the destination register for the request
             */
            RegType getDestinationRegType() const {return regType;}            

            /*!
             * \brief Sets the source tile of the request
             */
            void setSourceTile(uint16_t source)
            {
                source_tile=source;
            }


            /*!
             * \bref Sets the home tile of the request
             */
            void setHomeTile(uint16_t home)
            {
                home_tile=home;
            }


            /*!
             * \brief Sets the bank that will be accessed by the request
             */
            void setCacheBank(uint16_t b)
            {
                cache_bank=b;
            }

            void calculateLineAddress(uint8_t block_offset_bits)
            {
                line_address=(address >> block_offset_bits) << block_offset_bits;
            }

            /*!
             * \brief Calculates the home tile for the request
             * \note d is the data mapping policy to be used in the calculation
             *       tag_bits, block_offset_bits, set_bits, bank_bits define the geometry of the L2 cache that will be accessed
             */
            uint16_t calculateHome(CacheDataMappingPolicy d, uint8_t tag_bits, uint8_t block_offset_bits, uint8_t set_bits, uint8_t bank_bits);


            /*!
             * \brief Returns the home tile for the request
             */
            uint16_t getHomeTile(){return home_tile;}

            /*!
             * \brief Returns the source tile for the request
             */
            uint16_t getSourceTile(){return source_tile;}

            /*!
             * \brief Returns the bank that will be accessed by the request
             */
            uint16_t getCacheBank(){return cache_bank;}

            /*!
             * \brief Returns the program counter for the requesting instruction
             */
            uint64_t getPC(){return pc;}
            
            uint64_t getLineAddress(){return line_address;}
        
            void setMemoryAccessInfo(uint64_t memory_controller, uint64_t rank, uint64_t bank, uint64_t row, uint64_t col);
            
            uint64_t getMemoryController();
            uint64_t getRank();
            uint64_t getMemoryBank();
            uint64_t getRow();
            uint64_t getCol();

            bool operator ==(const Request & m) const
            {
                return m.getAddress()==getAddress();
            }

        private:
            uint64_t address;

            uint64_t line_address;

            AccessType type;
            uint64_t pc;
            uint64_t timestamp;
            uint16_t coreId;
            uint8_t regId;
            RegType regType;

            uint16_t home_tile;
            uint16_t source_tile;
            uint16_t cache_bank;

            uint64_t memory_controller_;
            uint64_t rank_;
            uint64_t memory_bank_;
            uint64_t row_;
            uint64_t col_;
    };
    
    inline std::ostream & operator<<(std::ostream & Str, Request const & req)
    {
        Str << "0x" << std::hex << req.getAddress() << " @ " << req.getTimestamp();
        return Str;
    }
}
#endif
