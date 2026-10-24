#ifndef CONTROL_INSTRUCTIONS
#define CONTROL_INSTRUCTIONS

#include <format>

#include "rvsim/instruction.hpp"
#include "rvsim/fields.hpp"
#include "rvsim/register_file.hpp"

namespace rvsim {

template <typename Compare, typename T>
class BranchInstruction final : public Instruction {
    static_assert(std::is_same_v<T, std::int32_t> || std::is_same_v<T, std::uint32_t>,
                  "a branch compares two 32-bit values, signed or unsigned");
    public:
        BranchInstruction(Addr pc, Word word) : 
                    Instruction(pc, word) {
            // set the instructions characteristics
            _reads_rs1 = true;
            _reads_rs2 = true;
            _is_control_flow = true;

            _opcode = rvsim::opcode(_word);

            // get the source and destination registers
            _rs1 = rvsim::rs1(_word);
            _rs2 = rvsim::rs2(word);

            _imm = rvsim::imm_b(_word);

            // instruction kind 
            _kind = InstructionKind::Branch;
        }

        //return the human readable instruction
        std::string disassemble() const override {
            return std::format("{}  {}, {}, {}", mnemonic(),
                                                    rvsim::abi_name(_rs1),
                                                    rvsim::abi_name(_rs2),
                                                    _imm);;
        }

        InstructionKind kind() const override {
            return _kind;
        }

        std::string mnemonic() const override {
            if constexpr (std::is_same_v<Compare, std::equal_to<>>)
                return "beq";
            else if constexpr (std::is_same_v<Compare, std::not_equal_to<>>)
                return "bne";
            else if constexpr (std::is_same_v<Compare, std::less<>>)
                return std::is_unsigned_v<T> ? "bltu" : "blt";
            else
                return std::is_unsigned_v<T> ? "bgeu" : "bge";
        }
    private:
        // helper function to extract everything
        void extract_fields() noexcept;

};

class JalInstruction final : public Instruction {

    public:
        JalInstruction(Addr pc, Word word) : 
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

class JalrInstruction final : public Instruction {

    public:
        JalrInstruction(Addr pc, Word word) : 
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