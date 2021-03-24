#ifndef __FIFO_MEMORY_ACCESS_SCHEDULER_HBM_HH__
#define __FIFO_MEMORY_ACCESS_SCHEDULER_HBM_HH__

#include <queue>
#include <vector>
#include <list>
#include "Request.hpp"
#include "FifoRrMemoryAccessScheduler.hpp"

namespace spike_model {
    class FifoRrMemoryAccessSchedulerHBM : public MemoryAccessSchedulerIF {
        /*!
         * \class spike_model::FifoRrMemoryAccessSchedulerHBM
         * \brief A memory access scheduler that follows the policy as described in the
	 * reference manual for the HBM integrated in the Alveo U280. Please refer to
	 * "AXI High BandwidthMemory Controller v1.0" (PG276, January 2021)
	 * https://www.xilinx.com/support/documentation/ip_documentation/hbm/v1_0/pg276-axi-hbm.pdf
	 *
	 * HBM is available in 2 options: 4GB or 8GB per stack
	 *  - per stack 1024 bit bus width, divided across 8 channels (128bit each)
	 *  - each channel divided into 2 pseudo channels:
	 *  	- 64 bit each
	 *  	- shared command
	 *  - HBM is DDR: data bus toggles twice as fast as interface clock rate
	 *
	 *
	 *  Address Scheme:
	 *
	 *  8GB per stack
	 *
	 *                    33 32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
	 * Row Bank Column     | | AXI Port| SID|<---          RA13 - RA0           --->| |BG | |BA | |<  CA5-CA1 >| |<- unused ->|
	 * (standard option)   | | 0 - 15  |                                              |1:0| |1:0|
	 *                     + stack selector (0 = left, 1 = right)
	 *
	 * Row Bank Column     | | AXI Port| SID|<---          RA13 - RA0           --->| |B||BA | |<  CA5-CA1 >| |B||<- unused ->|
	 * (bank interleave)   | | 0 - 15  |                                              |G||1:0|                |G|
	 *                     + stack selector (0 = left, 1 = right)                     |1|                     |0|
	 *
	 *  4GB per stack
	 *
	 *                    32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
	 * Row Bank Column     | | AXI Port| |<---          RA13 - RA0           --->| |BG | |BA | |<  CA5-CA1 >| |<- unused ->|
	 * (standard option)   | | 0 - 15  |                                           |1:0| |1:0|
	 *                     + stack selector (0 = left, 1 = right)
	 *
	 * Row Bank Column     | | AXI Port| |<---          RA13 - RA0           --->| |B||BA | |<  CA5-CA1 >| |B||<- unused ->|
	 * (bank interleave)   | | 0 - 15  |                                           |G||1:0|                |G|
	 *                     + stack selector (0 = left, 1 = right)                  |1|                     |0|
	 *
	 * Key:
	 * 	RA - Row Address
	 * 	CA - Column Address
	 * 	BG - Bank Group Address
	 * 	BA - Bank Address
	 * Other options: Row Column Bank, Bank Row Column, 14R-2BA-1BG-5C-1BG, 2BA-14R-1BG-5C-1BG
	 *
         */
        public:

            /*!
            * \brief Constructor for FifoRrMemoryAccessScheduler
            * \param num_banks The number of banks handled by the scheduler
            */
            FifoRrMemoryAccessSchedulerHBM(uint64_t num_banks);

            /*!
            * \brief Add a request to the scheduler
            * \param req The request
            * \param bank The bank that the request targets
            */
            void putRequest(std::shared_ptr<Request> req, uint64_t bank) override;

            /*!
            * \brief Get a request for a particular bank
            * \param bank The bank for which the request is desired
            * \return The first request for the bank in FIFO
            */
            std::shared_ptr<Request> getRequest(uint64_t bank) override;

            /*!
            * \brief Get the next bank for which a request should be handled. The bank is poped from the list.
            * \return The next bank in round-robin (out of the banks with requests)
            */
            uint64_t getNextBank() override;

            /*!
            * \brief Check there is any bank with pending requests that is idle
            * \return True if a request for a bank can be handled
            */
            virtual bool hasIdleBanks() override;

            /*!
            * \brief Notify that a request has been completed
            * \param bank The bank which completed the request
            */
            void notifyRequestCompletion(uint64_t bank) override;

        private:
            std::vector<std::list<std::shared_ptr<Request>>> request_queues;
            std::queue<uint64_t> banks_to_schedule; //A bank id will be in this queue if the associated bank queue has elements and a request to the buffer is not currently in process
    };
}
#endif
