#ifndef __ACK_HH__
#define __ACK_HH__

namespace minimum_two_phase_example
{
    class Req;

    class Ack
    {
        public:

            Ack(std::shared_ptr<Req> req) : req(req) {}

            std::shared_ptr<Req> getReq() const {return req;}

        private:
            std::shared_ptr<Req> req;
    };
}
#endif
