
#include "sparta/utils/SpartaAssert.hpp"
#include "L3CacheBank.hpp"
#include <chrono>

namespace spike_model
{
    const char L3CacheBank::name[] = "l3";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    L3CacheBank::L3CacheBank(sparta::TreeNode *node, const L3CacheBankParameterSet *p) :
                    CacheBank(node, p->always_hit, p->miss_latency, p->hit_latency,
                              p->max_outstanding_misses, p->max_outstanding_wbs, false, p->unit_test, p->line_size, p->size_kb,
                              p->associativity, p->bank_and_tile_offset)
    {
        in_core_req_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(L3CacheBank, getAccess_, std::shared_ptr<Request>));

        in_biu_ack_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(L3CacheBank, sendAck_, std::shared_ptr<CacheRequest>));
    }
    
    void L3CacheBank::logCacheRequest(std::shared_ptr<CacheRequest> r)
    {
        switch(r->getType())
        {
            case CacheRequest::AccessType::FETCH:
                logger_->logLLCRead(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress(), r->getSize());
                break;

            case CacheRequest::AccessType::LOAD:
                logger_->logLLCRead(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress(), r->getSize());
                break;

            case CacheRequest::AccessType::STORE:
                logger_->logLLCWrite(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress(), r->getSize());
                break;

            case CacheRequest::AccessType::WRITEBACK:
                logger_->logLLCWrite(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress(), r->getSize());
                break;
        }

    }

    void L3CacheBank::scheduleIssueAccess(uint64_t cycle)
    {
        issue_access_event_.schedule(cycle);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Callbacks
    ////////////////////////////////////////////////////////////////////////////////

    // Receive MSS access acknowledge from Bus Interface Unit
    void L3CacheBank::sendAck_(const std::shared_ptr<CacheRequest> & req)
    {
        sendAckInternal_(req);
    }

    void L3CacheBank::issueAccess_()
    {
        issueAccessInternal_();
    }

    void L3CacheBank::getAccess_(const std::shared_ptr<Request> & req)
    {
        CacheBank::getAccess_(req);
    }
} // namespace core_example
