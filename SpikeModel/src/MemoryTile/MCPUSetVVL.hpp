#ifndef __MCPU_SetVVL_HH__
#define __MCPU_SetVVL_HH__

#include "EventVisitor.hpp"
#include "Request.hpp"
#include "VectorElementType.hpp"
#include <iostream>

namespace spike_model
{
    class MCPUSetVVL : public Request, public std::enable_shared_from_this<MCPUSetVVL>
    {
        /**
         * \class spike_model::MCPUSetVVL
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
            MCPUSetVVL(uint64_t pc):Request(pc, 0, 0) {}

            /*!
             * \brief Constructor for MCPUSetVVL
             * \param avl The application vector length set by scalar core
             * \param regId Destination register
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp when the event is submitted to sparta
             * \param c The core which submitted the event
             */
            MCPUSetVVL(uint64_t avl, size_t regId, uint64_t pc, uint64_t time, uint16_t c): Request(pc, time, c)
                          , avl(avl) {
                setLMUL(1);
                setDestinationReg(regId);
                setWidth(VectorElementType::BIT64);
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
                Request::setDestinationReg(r, RegType::INTEGER);
            }

            
            /*!
             * \brief Set the LMUL setting for the current VVL request.
             * \param lmul, the lmul setting.
             */
            void setLMUL(uint8_t lmul) {this->lmul = lmul;}
            
            /*!
             * \brief Return the LMUL setting
             * \return The LMUL setting for this request.
             */
            uint8_t getLMUL() {return lmul;}
            
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
            uint8_t lmul;
            VectorElementType width;
            uint64_t avl, vvl;
    };
    
    
    inline std::ostream& operator<<(std::ostream &str, MCPUSetVVL &instr) {
        str << "AVL: " << instr.getAVL() << ", VVL: " << instr.getVVL() << ", lmul: " << instr.getLMUL() << "w: " << (uint)instr.getWidth() << ", coreID: " << instr.getCoreId();
        return str;
    }
}
#endif
