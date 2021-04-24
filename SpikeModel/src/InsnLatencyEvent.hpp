#ifndef __INSN_LATENCY_EVENT_HH__
#define __INSN_LATENCY_EVENT_HH__

#include "Request.hpp"
#include <iostream>

namespace spike_model
{
    class InsnLatencyEvent : public Request, public std::enable_shared_from_this<InsnLatencyEvent>
    {
        /**
         * \class spike_model::InsnLatencyEvent
         * \brief InsnLatencyEvent contains all the information regarding the details of RAW.
         *
         */
        public:
            InsnLatencyEvent() = delete;
            InsnLatencyEvent(Request const&) = delete;
            InsnLatencyEvent& operator=(Request const&) = delete;

            /*!
             * \brief Constructor for InsnLatencyEvent
             * \param  coreId The requesting core id
             * \param  srcRegId The source register id for this request
             * \param  regType The type of the register
             * \param  destRegId The destination register id for this request
               \param  the latency of the current instruction
               \param  availCycle The timestamp in which the reg_t would be available
             */
            InsnLatencyEvent(uint16_t coreId, uint64_t srcRegId,
                             spike_model::Request::RegType srcRegType,
                             uint64_t destRegId, uint64_t insn_latency,
                             uint64_t avail_cycle):
                             Request(coreId, destRegId),
                             avail_cycle(avail_cycle),
                             insn_latency(insn_latency),
                             srcRegId(srcRegId),
                             srcRegType(srcRegType){}

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override
            {
                v->handle(shared_from_this());
            }

            uint64_t getAvailCycle()
            {
                return avail_cycle;
            }

            uint64_t getInsnLatency()
            {
                return insn_latency;
            }

            uint64_t getSrcRegId()
            {
                return srcRegId;
            }

            spike_model::Request::RegType getSrcRegType()
            {
                return srcRegType;
            }

        private:
            uint64_t avail_cycle;
            uint64_t insn_latency;
            uint64_t srcRegId;
            spike_model::Request::RegType srcRegType;
    };
}
#endif
