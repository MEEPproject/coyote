
#ifndef __BANK_COMMAND_HH__
#define __BANK_COMMAND_HH__

#include <Request.hpp>

namespace spike_model
{
    class BankCommand
    {
        public:
            enum class CommandType
            {
                OPEN,
                CLOSE,
                READ,
                WRITE
            };

            BankCommand() = delete;
            BankCommand(BankCommand const&) = delete;
            BankCommand& operator=(BankCommand const&) = delete;
            
            /*!
             * \brief Constructor for BankCommand
             *        t is the type of the command
             *        payload_size is the size in bytes of the data to be sent
             */
            BankCommand(CommandType t, uint64_t bank, uint64_t param)
            {
                type=t;
                value=param;
                destination_bank=bank;
            }

            CommandType getType()
            {
                return type;
            }

            uint64_t getValue()
            {
                return value;
            }

            uint64_t getDestinationBank()
            {
                return destination_bank;
            }

        private:
            CommandType type;
            uint64_t value;
            uint64_t destination_bank;
    };
}
#endif
