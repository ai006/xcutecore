#ifndef LOAD_STORE_INSTRUCTIONS_HPP
#define LOAD_STORE_INSTRUCTIONS_HPP

#include <concepts>
#include <format>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "rvsim/fields.hpp"
#include "rvsim/register_file.hpp"

namespace rvsim {

template <typename T>
class LoadInstruction final : public Instruction {

    static_assert(std::is_integral_v<T> && (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4),
                "a load reads 1, 2, or 4 bytes");

    public:

        LoadInstruction(Addr pc, Word word) : 
                    Instruction(pc, word) {

            //set the instruction flags
            _reads_rs1 = true;
            _writes_rd = true;
            _is_load = true;

            // get the source and destination registers
            _rs1 = rvsim::rs1(_word);
            _rd = rvsim::rd(_word);

            _imm = rvsim::imm_i(_word);

            _kind = InstructionKind::Load;
            
        }

        InstructionKind kind() const override {
            return _kind;
        }
        // Built from T at compile time: l, then b, h, or w for the width, then u if unsigned.
        [[nodiscard]] std::string mnemonic() const override {
            std::string name = "l";
            if constexpr (sizeof(T) == 1)
                name += 'b';
            else if constexpr (sizeof(T) == 2)
                name += 'h';
            else
                name += 'w';
            if constexpr (std::is_unsigned_v<T>)
                name += 'u';
            return name;
        }

        // 5.6: mnemonic rd,imm(rs1), the offset in signed decimal: lw a5,8(sp).
        [[nodiscard]] std::string disassemble() const override {
            return   std::format("{} , {}, {}({})", _mnemonic,
                                            rvsim::abi_name(_rs2),
                                            rvsim::abi_name(_rs1),
                                            _imm);
        }
};

template <typename T>
class StoreInstruction final : public Instruction {
    static_assert(std::is_unsigned_v<T> && (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4),
                  "a store writes 1, 2, or 4 bytes");
                  
    public:
        StoreInstruction(Addr pc, Word word) :
                    Instruction(pc , word) {

                        //set the instruction flags
            _reads_rs1 = true;
            _reads_rs2 = true;

            // get the source and destination registers
            _rs1 = rvsim::rs1(_word);
            _rs2 = rvsim::rs2(_word);

            _imm = rvsim::imm_s(_word);

            _kind = InstructionKind::Store;


        }

        //return the human readable instruction
        std::string disassemble() const override {
            return std::format("{} , {}, {}({})", _mnemonic,
                                                rvsim::abi_name(_rs2),
                                                rvsim::abi_name(_rs1),
                                                _imm);;
        }

        InstructionKind kind() const override {
            return _kind;
        }

        std::string mnemonic() const override {
            if constexpr (sizeof(T) == 1)
                return "sb";
            else if constexpr (sizeof(T) == 2)
                return "sh";
            else
                return "sw";
        }
};


}

#endif