#ifndef __SCRATCHPAD_REQUEST_HH__
#define __SCRATCHPAD_REQUEST_HH__

#include "CacheDataMappingPolicy.hpp"
#include "Request.hpp"
#include <iostream>

namespace spike_model
{
    class ScratchpadRequest : public Request, public std::enable_shared_from_this<ScratchpadRequest>
    {
        /**
         * \class spike_model::ScratchpadRequest
         *
         * \brief ScratchpadRequest contains all the information regarding a request to the L2 cache.
         *
         * Instances of request are the main data structure that is communicated between Spike and the Sparta model
         * and between Sparta Units, either as plain ScratchpadRequests or encapsulated in a NoCMessage.
         *
         */
        
        friend class AccessDirector; 
    
        public:
            enum class ScratchpadCommand
            {
                ALLOCATE,
                FREE,
                READ,
                WRITE,
            };
            
            //ScratchpadRequest(){}
            ScratchpadRequest() = delete;
            ScratchpadRequest(Request const&) = delete;
            ScratchpadRequest& operator=(Request const&) = delete;

            /*!
             * \brief Constructor for ScratchpadRequest
             * \param a The address for the request
             * \param comm The command for the scratchpad
             * \param pc The program counter of the instruction related to this scratchpad request
             * \param time The timestamp for the request
             * \param c The ID of the destination core
             * \param src The ID of the Memory Tile that is generating this ScratchpadRequest
             * \param destReg The destination register
             */
            ScratchpadRequest(uint64_t a, ScratchpadCommand comm, uint64_t pc, uint64_t time, uint16_t c, uint16_t src, uint16_t destReg): Request(a, pc, time, c), command(comm) {
            setDestinationReg(destReg, RegType::VECTOR);
            setSourceTile(src);
            //This constructor will have an extra parameter: The memory instruction that triggered the request. The pc would then be redundant, but we need it for the Event class hierarchy
            }
            
            /*!
             * \brief Constructor for ScratchpadRequest
             * \param a The address for the request
             * \param comm The command for the scratchpad
             * \param pc The program counter of the instruction related to this scratchpad request
             * \param time The timestamp for the request
             * \param ready True if the this request completes an operand and the associated instruction may proceed
             */
            ScratchpadRequest(uint64_t a, ScratchpadCommand comm, uint64_t pc, uint64_t time, bool ready): Request(a, pc), command(comm), operand_ready(ready)
            {
                setTimestamp(time);
                //This constructor will have an extra parameter: The memory instruction that triggered the request. The pc would then be redundant, but we need it for the Event class hierarchy
            }

            /*!
             * \brief Get the type of command
             * \return The type of command
             */
            ScratchpadCommand getCommand() const {return command;}


            /*
             * \brief Equality operator for instances of ScratchpadRequest
             * \param m The request to compare with
             * \return true if both requests are for the same address
             */
            bool operator ==(const ScratchpadRequest & m) const
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

            /*!
             * \brief Check if the associated operand will become ready
             * \return True if an operand will become ready after servicing this request
             */
            bool isOperandReady() const {return operand_ready;}
            void setOperandReady() {operand_ready = true;}

        private:
            ScratchpadCommand command;
            bool operand_ready;
    };
    
    inline std::ostream & operator<<(std::ostream & Str, ScratchpadRequest const & req)
    {
        Str << "0x" << std::hex << req.getAddress() << ", type: " << (int)req.getCommand() << " @ " << req.getTimestamp();
        return Str;
    }
}
#endif
