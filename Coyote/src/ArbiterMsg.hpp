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

#ifndef __ARBITER_MESSAGE_HH__
#define __ARBITER_MESSAGE_HH__

namespace coyote
{

enum class MessageType
{
    NOC_MSG,
    CACHE_REQUEST
};

typedef struct ArbiterMessage
{
    std::shared_ptr<void> msg;
    bool is_core;
    int id;
    MessageType type;
}ArbiterMessage;

}
#endif
