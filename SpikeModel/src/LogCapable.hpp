#ifndef __LOG_CAPABLE_HH__
#define __LOG_CAPABLE_HH__

#include <memory>
#include <fstream>

#include <Logger.hpp>

namespace spike_model
{
    class LogCapable
    {
        public:
            void setLogger(Logger & l)
            {
                trace_=true;
                logger_=l;
            }

        protected:
            bool trace_=false;
            Logger logger_;
    };
}
#endif
