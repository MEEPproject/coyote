#ifndef __MCPURequest_HH__
#define __MCPURequest_HH__

#include "EventVisitor.hpp"
#include <iostream>

namespace spike_model
{
    class MCPURequest : public Event, public std::enable_shared_from_this<MCPURequest>
    {
        /**
         * \class spike_model::Fence
         *
         * \brief Fence signals that the Spike simulation has been completed.
         *
         */

        public:

            //Fence(){}
            MCPURequest() = delete;
            MCPURequest(MCPURequest const&) = delete;
            MCPURequest& operator=(MCPURequest const&) = delete;

            /*!
             * \brief Constructor for Fence
             * \param pc The program counter of the instruction that finished the simulation
             */
            MCPURequest(uint64_t pc):Event(pc)
            {}

            /*!
             * \brief Constructor for Fence
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The finishing core
             */
            MCPURequest(uint64_t reqVL, uint64_t pc, uint64_t time, uint16_t c): Event(pc, time, c)
            {
                this->reqVL = reqVL;
            }

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override
            {
                v->handle(shared_from_this());
            }

            uint64_t getRequestedVecLen()
            {
                return reqVL;
            }

            uint64_t getReturnedVecLen()
            {
                return returnedVL;
            }

            void setReturnedVecLen(uint64_t returnedVL)
            {
                this->returnedVL = returnedVL;
            }

            /*!
             * \brief Get the memory CPU that will be accessed
             * \return The address of the line              */
            uint64_t getMemoryCPU()
            {
                return memoryCPU;
            }

        private:
            uint64_t reqVL, returnedVL, memoryCPU;

    };
}
#endif
