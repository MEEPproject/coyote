#include "FifoRrMemoryAccessSchedulerHBM.hpp"

namespace spike_model
{
    FifoRrMemoryAccessSchedulerHBM::FifoRrMemoryAccessSchedulerHBM(uint64_t num_banks) : request_queues(num_banks){}

    void FifoRrMemoryAccessSchedulerHBM::putRequest(std::shared_ptr<Request> req, uint64_t bank) {

		uint64_t request_row = req->getRow();

		//-- search in the command queue of the respected bank, if there are already commands addressing the same row.
		//   if there is a request, squeeze the new one in between
		//   if there is none, add this request at the end of the queue
		//
		//   FIXME: Maybe there is a more efficient way to go through that array. vectors are slower
		//   than lists, if an element is inserted somewhere in the middle.

		bool found = false;
		//-- walk through the list from its end
		for(auto it=rbegin(request_queues[bank]), it_end=rend(request_queues[bank]); it!=it_end; ++it) {
			if((*it)->getRow() == request_row) {
				request_queues[bank].insert(it.base(), req);	// insert behind the current element, so that
																// dependencies are preserved.
				found = true;
				break;
			}
		}
		if(!found) {
			request_queues[bank].push_back(req);
		}

        if(request_queues[bank].size()==1) {
            banks_to_schedule.push(bank);
        }
    }

    std::shared_ptr<Request> FifoRrMemoryAccessSchedulerHBM::getRequest(uint64_t bank)
    {
        std::shared_ptr<Request> res=request_queues[bank].front();
        return res;
    }

    uint64_t FifoRrMemoryAccessSchedulerHBM::getNextBank()
    {
        uint64_t res=banks_to_schedule.front();
        banks_to_schedule.pop();
        return res;
    }

    bool FifoRrMemoryAccessSchedulerHBM::hasIdleBanks()
    {
        return banks_to_schedule.size()>0;
    }

    void FifoRrMemoryAccessSchedulerHBM::notifyRequestCompletion(uint64_t bank)
    {
        request_queues[bank].pop_front();
        if(request_queues[bank].size()>0) {
            banks_to_schedule.push(bank);
        }
    }
}
// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab:
