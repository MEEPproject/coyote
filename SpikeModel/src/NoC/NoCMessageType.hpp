
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
}
#endif
