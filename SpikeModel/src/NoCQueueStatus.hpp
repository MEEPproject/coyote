#ifndef __NOC_QUEUE_STATUS_HH__
#define __NOC_QUEUE_STATUS_HH__

#include "EventVisitor.hpp"
#include <iostream>

namespace spike_model
{
    class NoCQueueStatus : public Event, public std::enable_shared_from_this<NoCQueueStatus>
    {
        /**
         * \class spike_model::NoCQueueStatus
         *
         * \brief NoCQueueStatus creates the event to indicate if NoC is full/empty
         *
         */

        public:

            NoCQueueStatus() = delete;
            NoCQueueStatus(NoCQueueStatus const&) = delete;
            NoCQueueStatus& operator=(NoCQueueStatus const&) = delete;

            /*!
             * \brief Constructor for NoCQueueStatus
             * \param pc The program counter of the instruction that generates the event
             */
            NoCQueueStatus(uint16_t c):Event(c)
            {}

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override
            {
                v->handle(shared_from_this());
            }
    };
}
#endif
