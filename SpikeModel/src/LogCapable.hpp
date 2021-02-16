#ifndef __LOG_CAPABLE_HH__
#define __LOG_CAPABLE_HH__

#include <memory>
#include <fstream>

#include <Logger.hpp>

namespace spike_model
{
    /*!
     * \class spike_model::LogCapable
     * \brief An element that has access to a Logger and can consequently write 
     *  information to the execution trace.
     */
    class LogCapable
    {
        public:
            /*!
             * \brief Set the logger that will be used for tracing.
             * \param l The logger to use
             */
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
