#ifndef __REQ_HH__
#define __REQ_HH__

namespace minimum_two_phase_example
{
    class Req
    {
        public:
            enum class AccessType
            {
                LOAD,
                STORE,
                FETCH,
                WRITEBACK,
            };

            Req(uint64_t address, uint64_t size, AccessType type) : address(address), size(size), type(type) {}

            uint64_t getAddress() const {return address;}

            uint16_t getSize() const {return size;}

            AccessType getType()const {return type;}

        private:
            uint64_t address;
            uint16_t size;
            AccessType type;
    };
}
#endif
