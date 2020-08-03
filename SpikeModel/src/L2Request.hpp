#ifndef __MEMORY_ACCESS_HH__
#define __MEMORY_ACCESS_HH__


namespace spike_model
{
    class L2Request
    {
        public:
            enum class AccessType
            {
                LOAD,
                STORE,
                FETCH,
                WRITEBACK,
                FINISH 
            };
            
            enum class RegType
            {
                INTEGER,
                FLOAT,
                VECTOR,
            };

            L2Request(){}
            L2Request(uint64_t a, size_t s, AccessType t): address(a), size(s), type(t){}
            L2Request(uint64_t a, size_t s, AccessType t, uint64_t time, uint16_t c): address(a), size(s), type(t), timestamp(time), coreId(c){}

            uint64_t getAddress() const {return address;}
            size_t getSize(){return size;}
            AccessType getType() const {return type;}
            uint64_t getTimestamp() const {return timestamp;}
            uint64_t getCoreId() const {return coreId;}
            void setTimestamp(uint64_t t) {timestamp=t;}
            void setCoreId(uint16_t c) {coreId=c;}
            uint8_t getRegId() const {return regId;}
            void setRegId(uint8_t r, RegType t) {regId=r; regType=t;}
            RegType getRegType() const {return regType;}            

            bool operator ==(const L2Request & m) const
            {
                return m.getAddress()==getAddress();
            }

        private:
            uint64_t address;
            size_t size;
            AccessType type;
            uint64_t timestamp;
            uint16_t coreId;
            uint8_t regId;
            RegType regType;
    };
    
    inline std::ostream & operator<<(std::ostream & Str, L2Request const & req)
    {
        Str << "0x" << std::hex << req.getAddress() << " @ " << req.getTimestamp();
        return Str;
    }
}
#endif
