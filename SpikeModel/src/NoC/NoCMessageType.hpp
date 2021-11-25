#include<iostream>
#include "sparta/utils/SpartaAssert.hpp"

#ifndef __NOC_MESSAGE_TYPE_HH__
#define __NOC_MESSAGE_TYPE_HH__


namespace spike_model
{
    /**
     * This class defines the types of messages that the NoC is able to communicate. Have a look at the wiki
     * about the sequence of and more details about the messages.
     * Please note, that the NoC considers not only the source/destination pair, but also the message
     * type. For instance, an MCPU_REQUEST can be send from source 0 to destination 0. However the NoC
     * knows, which id defines destination 0, since an MCPU Request can only be sent from a VAS Tile
     * to a Memory Tile. Therefore, this MCU_REQUEST will be picked up from VAS TIle 0 and dropped off at
     * Memory Tile 0.
     * @see https://wiki.meep-project.eu/index.php/The_Coyote_simulator#NoC_Implementation
     */
    enum class NoCMessageType
    {
        REMOTE_L2_REQUEST       = 0,
        MEMORY_REQUEST_LOAD     = 1,
        MEMORY_REQUEST_STORE    = 2,
        MEMORY_REQUEST_WB       = 3,
        REMOTE_L2_ACK           = 4,
        
        /**
         * Once the memory operation has been completed, the Memory Tile replies with an MEMORY_ACK
         * to the VAS Tile.
         */
        MEMORY_ACK              = 5,
        
        /**
         * An instruction from the VAS Tiles to the Memory Tile. It can be a scalar as well as vector
         * memory operation.
         */
        MCPU_REQUEST            = 6,
        
        /**
         * This is a reply from the Scratchpad to the Memory Tile indicating that a command has been
         * completed.
         */
        SCRATCHPAD_ACK          = 7,
        
        /**
         * If the MemoryTile receives a vector store operation, it needs to issue a read from the scratchpad. 
         * To do so, it uses SCRATCHPAD_COMMAND with the operation "READ" set.
         * The Scratchpad returns VVL elements (in several transactions) via SCRATCHPAD_DATA_ACK acknowledging
         * the execution of the command at the same time.
         */
        SCRATCHPAD_DATA_REPLY   = 8,
        
        /**
         * The command the Memory Tile, it wants the Scratchpad execute, such as ALLOC, DEALLOC, READ, or WRITE.
         */
        SCRATCHPAD_COMMAND      = 9,
        
        /**
         * The Requests that are sent among the Memory Tiles.
         */
        MEM_TILE_REQUEST        =10,
        
        /**
         * Replies/Acknowledgements for the MEM_TILE_REQUEST
         */
        MEM_TILE_REPLY          =11,
        
        count                   =12 // Number of message types
    };

    inline std::ostream& operator<<(std::ostream& os, NoCMessageType nmt)
    {
        switch(nmt)
        {
            case NoCMessageType::REMOTE_L2_REQUEST: os << "REMOTE_L2_REQUEST"; return os;
            case NoCMessageType::MEMORY_REQUEST_LOAD: os << "MEMORY_REQUEST_LOAD"; return os;
            case NoCMessageType::MEMORY_REQUEST_STORE: os << "MEMORY_REQUEST_STORE"; return os;
            case NoCMessageType::MEMORY_REQUEST_WB: os << "MEMORY_REQUEST_WB"; return os;
            case NoCMessageType::REMOTE_L2_ACK: os << "REMOTE_L2_ACK"; return os;
            case NoCMessageType::MEMORY_ACK: os << "MEMORY_ACK"; return os;
            case NoCMessageType::MCPU_REQUEST: os << "MCPU_REQUEST"; return os;
            case NoCMessageType::SCRATCHPAD_ACK: os << "SCRATCHPAD_ACK"; return os;
            case NoCMessageType::SCRATCHPAD_DATA_REPLY: os << "SCRATCHPAD_DATA_REPLY"; return os;
            case NoCMessageType::SCRATCHPAD_COMMAND: os << "SCRATCHPAD_COMMAND"; return os;
            case NoCMessageType::MEM_TILE_REQUEST: os << "MEM_TILE_REQUEST"; return os;
            case NoCMessageType::MEM_TILE_REPLY: os << "MEM_TILE_REPLY"; return os;
            default: os << "UNKNOWN_NoCMessageType"; return os;
        }
    }

    inline std::string& operator+(std::basic_string<char> str, NoCMessageType nmt)
    {
        switch(nmt)
        {
            case NoCMessageType::REMOTE_L2_REQUEST: return str.append("REMOTE_L2_REQUEST");
            case NoCMessageType::MEMORY_REQUEST_LOAD: return str.append("MEMORY_REQUEST_LOAD");
            case NoCMessageType::MEMORY_REQUEST_STORE: return str.append("MEMORY_REQUEST_STORE");
            case NoCMessageType::MEMORY_REQUEST_WB: return str.append("MEMORY_REQUEST_WB");
            case NoCMessageType::REMOTE_L2_ACK: return str.append("REMOTE_L2_ACK");
            case NoCMessageType::MEMORY_ACK: return str.append("MEMORY_ACK");
            case NoCMessageType::MCPU_REQUEST: return str.append("MCPU_REQUEST");
            case NoCMessageType::SCRATCHPAD_ACK: return str.append("SCRATCHPAD_ACK");
            case NoCMessageType::SCRATCHPAD_DATA_REPLY: return str.append("SCRATCHPAD_DATA_REPLY");
            case NoCMessageType::SCRATCHPAD_COMMAND: return str.append("SCRATCHPAD_COMMAND");
            case NoCMessageType::MEM_TILE_REQUEST: return str.append("MEM_TILE_REQUEST");
            case NoCMessageType::MEM_TILE_REPLY: return str.append("MEM_TILE_REPLY");
            default: sparta_assert(false);
        }
    }
}
#endif
