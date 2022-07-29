// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 

#ifndef __LOG_CAPABLE_HH__
#define __LOG_CAPABLE_HH__

#include <memory>
#include <fstream>

#include <Logger.hpp>

namespace coyote
{
    /*!
     * \class coyote::LogCapable
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
            void setLogger(Logger * l)
            {
                trace_=true;
                logger_= l;
            }

        protected:
            bool trace_=false;
            Logger* logger_;
    };
}
#endif
