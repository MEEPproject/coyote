#ifndef __PARENT_INST_ID_HH__
#define __PARENT_INST_ID_HH__

namespace spike_model
{
    class ParentInstId
    {
        /**
         * \class spike_model::ParentInstId
         *
         * \brief ParentInstId contains all the information regarding an event to be communicated from Spike to the rest of Coyote.
         *
         * Instances of ParentInstId (ant its children) are the main data structure that is communicated between Spike and the Sparta model
         * and between Sparta Units.
         *
         */
        
        public:

            ParentInstId(ParentInstId const&) = delete;
            ParentInstId& operator=(ParentInstId const&) = delete;

            /*!
             * \brief Constructor for ParentInstId
             */
            ParentInstId(): id(0){}
 
            /*!
             * \brief Get the ID of the instruction.
             * We use that in the Memory Tile to map original instructions to generated ones.
             */
            uint32_t getID() {return id;}
            
            /*!
             * \brief Set the ID of the instruction.
             * \param The ID to be set.
             */
            void setID(uint32_t id) {this->id = id;}

        private:
            uint32_t id;    // used to identify the parent instruction

    };
    
}
#endif
