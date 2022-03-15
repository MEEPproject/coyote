#ifndef __GREEDY_FIFO_MEMORY_ACCESS_SCHEDULER_HH__
#define __GREEDY_FIFO_MEMORY_ACCESS_SCHEDULER_HH__

#include <queue>
#include <vector>
#include "CacheRequest.hpp"
#include "BankCommand.hpp"
#include "FifoRrMemoryAccessScheduler.hpp"

namespace spike_model
{
    class GreedyFifoRrMemoryAccessScheduler : public FifoRrMemoryAccessScheduler
    {
        /*!
         * \class spike_model::FifoRrMemoryAccessScheduler
         * \brief A simple memory access scheduler that follows the following policy.
         * 1. The next bank to schedule is chosen in round-robin
         * 2. The next request to schedule is chosen in FIFO
         */
        public:
 
            /*!
            * \brief Constructor for FifoRrMemoryAccessScheduler
            * \param b The banks handled by the scheduler
            * \param num_banks The number of banks
            * \param write_allocate True if writes need to allocate in cache
            */
            GreedyFifoRrMemoryAccessScheduler(std::shared_ptr<std::vector<MemoryBank *>> banks, uint64_t num_banks, bool write_allocate);
    
            std::shared_ptr<BankCommand> getCommand(uint64_t bank) override;

       protected: 
            
            /*!
             * \brief Mark a bank as ready for scheduling if it has any pending requests. If the request will generate read/write next, 
             * it will be greedily pushed to the front of the scheduling queue
             * \param bank The bank to reschedule
             */
            void rescheduleBank(uint64_t bank) override;
    };
}
#endif
