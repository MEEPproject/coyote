#ifndef __MCPU_SetVVL_HH__
#define __MCPU_SetVVL_HH__

#include "EventVisitor.hpp"
#include "RegisterEvent.hpp"
#include "VectorElementType.hpp"
#include <iostream>

namespace coyote
{
    class MCPUSetVVL : public RegisterEvent, public std::enable_shared_from_this<MCPUSetVVL>
    {
        /**
         * \class coyote::MCPUSetVVL
         *
         * \brief MCPUSetVVL passes the virtual vector length to MCPU.
         *
         */

        public:

            MCPUSetVVL() = delete;
            MCPUSetVVL(MCPUSetVVL const&) = delete;
            MCPUSetVVL& operator=(MCPUSetVVL const&) = delete;

            /*!
             * \brief Constructor for MCPUSetVVL
             * \param pc The program counter of the instruction that generates the event
             */
            MCPUSetVVL(uint64_t pc):RegisterEvent(0, pc, 0, -1, coyote::RegisterEvent::RegType::DONT_CARE) {
                setLMUL(LMULSetting::ONE);
                setWidth(VectorElementType::BIT64);
            }

            /*!
             * \brief Constructor for MCPUSetVVL
             * \param avl The application vector length set by scalar core
             * \param regId Destination register
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp when the event is submitted to sparta
             * \param c The core which submitted the event
             */
            MCPUSetVVL(uint64_t avl, size_t regId, uint64_t pc, uint64_t time, uint16_t c): RegisterEvent(0, pc, c, regId,  coyote::RegisterEvent::RegType::INTEGER)
                          , avl(avl) {
                setLMUL(LMULSetting::ONE);
                setWidth(VectorElementType::BIT64);
		setTimestamp(time);
            }

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override {
                v->handle(shared_from_this());
            }

            uint64_t getVVL() {return vvl;}

            void setVVL(uint64_t vvl) {this->vvl = vvl;}

            uint64_t getAVL() {return avl;}

            void setAVL(uint64_t avl) {this->avl = avl;}

            /*!
             * \brief Set the id and type of the destination register for the SetVVL request
             * \param r The id of the register
             */
            void setDestinationReg(size_t r) {
                RegisterEvent::setDestinationReg(r, RegType::INTEGER);
            }

            
            /*!
             * \brief Set the LMUL setting for the current VVL request.
             * \param lmul, the lmul setting.
             */
            void setLMUL(LMULSetting lmul) {this->lmul = lmul;}
            
            /*!
             * \brief Return the LMUL setting
             * \return The LMUL setting for this request.
             */
            LMULSetting getLMUL() {return lmul;}
            
            /*!
             * \brief Set the vector element width.
             * \param The width of the vector elements.
             */
            void setWidth(VectorElementType width) {this->width = width;}
            
            /*!
             * \brief Get the vector element width
             * \return The element width of the current VVL request.
             */
            VectorElementType getWidth() {return width;}

        private:
            LMULSetting lmul;
            VectorElementType width;
            uint64_t avl, vvl;
    };
    
    
    inline std::ostream& operator<<(std::ostream &str, MCPUSetVVL &instr) {
        str << "AVL: 0x" << std::hex << instr.getAVL() << ", VVL: 0x" << instr.getVVL() << ", lmul: 0x" << (int)instr.getLMUL() << ", w: 0x" << (uint)instr.getWidth() << ", coreID: 0x" << instr.getCoreId() << ", PC: 0x" << instr.getPC();
        return str;
    }
}
#endif
