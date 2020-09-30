
#ifndef __ServicedRequests_H__
#define __ServicedRequests_H__

#include <memory>

#include "L2Request.hpp"


namespace spike_model
{

    class ServicedRequests
    {
        public:

            ServicedRequests()
            {
                serviced_misses=std::make_shared<std::queue<std::shared_ptr<L2Request>>> ();
            }

            void addRequest(std::shared_ptr<L2Request> req)
            {
                serviced_misses->push(req);
            }

            std::shared_ptr<L2Request> getRequest()
            {
                std::shared_ptr<L2Request> res=serviced_misses->front();
                serviced_misses->pop();
                return res;
            }

            bool hasRequest()
            {
                return !serviced_misses->empty();
            }

        private:
           std::shared_ptr<std::queue<std::shared_ptr<L2Request>>> serviced_misses;
    };
}
#endif
