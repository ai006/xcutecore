#ifndef SYSTEM_INSTRUCTIONS_HPP
#define SYSTEM_INSTRUCTIONS_HPP

#include "rvsim/instruction.hpp"

namespace rvsim {

// system instructions ECALL and EBREAK
class SystemInstruction final : public Instruction {

    public:
        SystemInstruction(Addr pc, Word word) : 
                    Instruction(pc, word) {
            extract_fields();
        }

        //return the human readable instruction
        std::string disassemble() const override {
            return _disassemble;
        }
        InstructionKind kind() const override {
            return _kind;
        }
        std::string mnemonic() const override {
            return _mnemonic;
        }
    private:
        // helper function to extract everything
        void extract_fields() noexcept;
};


// fence instruction (memory barriers)
class FenceInstruction final : public Instruction {

    public:
        FenceInstruction(Addr pc, Word word) : 
                    Instruction(pc, word) {
            extract_fields();
        }

        //return the human readable instruction
        std::string disassemble() const override {
            return _disassemble;
        }
        InstructionKind kind() const override {
            return _kind;
        }
        std::string mnemonic() const override {
            return _mnemonic;
        }
    private:
        // helper function to extract everything
        void extract_fields() noexcept;
};

class IllegalInstruction final : public Instruction {

    public:
        IllegalInstruction(Addr pc, Word word) : 
                    Instruction(pc, word) {
            extract_fields();
        }

        //return the human readable instruction
        std::string disassemble() const override {
            return _disassemble;
        }
        InstructionKind kind() const override {
            return _kind;
        }
        std::string mnemonic() const override {
            return _mnemonic;
        }
    private:
        // helper function to extract everything
        void extract_fields() noexcept;
};





}


#endif