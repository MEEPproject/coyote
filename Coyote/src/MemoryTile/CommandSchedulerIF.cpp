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

#include "CommandSchedulerIF.hpp"
#include "MemoryController.hpp"

namespace coyote
{
    CommandSchedulerIF::CommandSchedulerIF(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks) : latencies(latencies), last_timestamp_per_command_type((int)BankCommand::CommandType::NUM_COMMANDS, 0), last_read_timestamp_per_bank(num_banks,0), last_write_timestamp_per_bank(num_banks,0), last_activate_timestamp_per_bank(num_banks,0), access_after_activate_per_bank(num_banks, false), last_precharge_timestamp_per_bank(num_banks,0){}
        
    std::shared_ptr<BankCommand> CommandSchedulerIF::getNextCommand(uint64_t currentTimestamp)
    {
        std::shared_ptr<BankCommand> res=selectCommand(currentTimestamp);
        if(res!=nullptr)
        {
            last_timestamp_per_command_type[(int)res->getType()]=currentTimestamp;
            switch(res->getType())
            {
                case BankCommand::CommandType::ACTIVATE:
                    last_activate_timestamp_per_bank[res->getDestinationBank()]=currentTimestamp;
                    access_after_activate_per_bank[res->getDestinationBank()]=false;
                    break;
                case BankCommand::CommandType::PRECHARGE:
                    last_precharge_timestamp_per_bank[res->getDestinationBank()]=currentTimestamp;
                    break;
                case BankCommand::CommandType::READ:
                    last_read_timestamp_per_bank[res->getDestinationBank()]=currentTimestamp;
                    access_after_activate_per_bank[res->getDestinationBank()]=true;
                    break;
                case BankCommand::CommandType::WRITE:
                    access_after_activate_per_bank[res->getDestinationBank()]=true;
                    last_write_timestamp_per_bank[res->getDestinationBank()]=currentTimestamp;
                default:
                    break;
            }
        }
        return res;
    }
            
    bool CommandSchedulerIF::checkTiming(std::shared_ptr<BankCommand> c, uint64_t currentTimestamp)
    {
        bool res=true;
        switch(c->getType())
        {
            case BankCommand::CommandType::ACTIVATE:
            {
                bool meet_t_activate_to_activate=(last_timestamp_per_command_type[(int)BankCommand::CommandType::ACTIVATE]==0 || currentTimestamp >= last_timestamp_per_command_type[(int)BankCommand::CommandType::ACTIVATE]+(*latencies)[(int)MemoryController::LatencyName::RRDS]);
                bool meet_t_activate_same_bank=(currentTimestamp >= last_activate_timestamp_per_bank[c->getDestinationBank()]+(*latencies)[(int)MemoryController::LatencyName::RC] || last_activate_timestamp_per_bank[c->getDestinationBank()]==0);
                bool meet_t_precharge_to_activate=(currentTimestamp >= last_precharge_timestamp_per_bank[c->getDestinationBank()]+(*latencies)[(int)MemoryController::LatencyName::RP] || last_precharge_timestamp_per_bank[c->getDestinationBank()]==0);
                res=meet_t_activate_to_activate && meet_t_activate_same_bank && meet_t_precharge_to_activate;
                break;
            }
            case BankCommand::CommandType::PRECHARGE:
            {
                bool meet_t_read_to_precharge=(last_timestamp_per_command_type[(int)BankCommand::CommandType::READ]==0 || currentTimestamp >= last_timestamp_per_command_type[(int)BankCommand::CommandType::READ]+(*latencies)[(int)MemoryController::LatencyName::RTP]);
                bool meet_t_write_to_precharge=(currentTimestamp >= last_write_timestamp_per_bank[c->getDestinationBank()]+(*latencies)[(int)MemoryController::LatencyName::WR]+(*latencies)[(int)MemoryController::LatencyName::WL]+burst_length || last_write_timestamp_per_bank[c->getDestinationBank()]==0);
                bool meet_t_activate_to_precharge=(currentTimestamp >= last_activate_timestamp_per_bank[c->getDestinationBank()]+(*latencies)[(int)MemoryController::LatencyName::RAS]+(*latencies)[(int)MemoryController::LatencyName::RP] || last_activate_timestamp_per_bank[c->getDestinationBank()]==0);
                res=meet_t_read_to_precharge && meet_t_activate_to_precharge && meet_t_write_to_precharge;
                break;
            }
            case BankCommand::CommandType::READ:
            {

                bool meet_t_read_to_read=(last_timestamp_per_command_type[(int)BankCommand::CommandType::READ]==0 || currentTimestamp >= last_timestamp_per_command_type[(int)BankCommand::CommandType::READ]+(*latencies)[(int)MemoryController::LatencyName::CCDS]);
                bool meet_t_activate_new_bank_to_read=(access_after_activate_per_bank[c->getDestinationBank()] || currentTimestamp >= last_activate_timestamp_per_bank[c->getDestinationBank()]+(*latencies)[(int)MemoryController::LatencyName::RCDRD] || last_activate_timestamp_per_bank[c->getDestinationBank()]==0);
                bool meet_t_write_to_read=(currentTimestamp >= last_write_timestamp_per_bank[c->getDestinationBank()]+(*latencies)[(int)MemoryController::LatencyName::WTRL]+(*latencies)[(int)MemoryController::LatencyName::WL]+burst_length || last_write_timestamp_per_bank[c->getDestinationBank()]==0);
                res=meet_t_read_to_read && meet_t_activate_new_bank_to_read && meet_t_write_to_read;
                break;
            }
            case BankCommand::CommandType::WRITE:
            {
                bool meet_t_write_to_write=(last_timestamp_per_command_type[(int)BankCommand::CommandType::WRITE]==0 || currentTimestamp >= last_timestamp_per_command_type[(int)BankCommand::CommandType::WRITE]+(*latencies)[(int)MemoryController::LatencyName::CCDS]);
                bool meet_t_read_to_write=(currentTimestamp >= last_read_timestamp_per_bank[c->getDestinationBank()]+(*latencies)[(int)MemoryController::LatencyName::RTW] || last_read_timestamp_per_bank[c->getDestinationBank()]==0);
                bool meet_t_activate_new_bank_to_write=(access_after_activate_per_bank[c->getDestinationBank()] || last_activate_timestamp_per_bank[c->getDestinationBank()]+(*latencies)[(int)MemoryController::LatencyName::RCDWR] || last_activate_timestamp_per_bank[c->getDestinationBank()]==0);
                res=meet_t_write_to_write && meet_t_read_to_write && meet_t_activate_new_bank_to_write;
                break;
            }
            case BankCommand::CommandType::NUM_COMMANDS:
                std::cout << "Messages of type NUM_COMMANDS should not be issued\n";
                break;

        }
        return res;
    }
    
    void CommandSchedulerIF::setBurstLength(uint8_t l)
    {
        burst_length=l;
    }
}
