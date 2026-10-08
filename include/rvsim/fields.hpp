#ifndef FIELDS_HPP
#define FIELDS_HPP

#include "rvsim/bits.hpp"
#include "rvsim/types.hpp"

namespace rvsim {

// Opcodes for all 40 of the RV32I
enum class Opcode : Word {

    LUI      = 0b0110111,
    AUIPC    = 0b0010111,
    JAL      = 0b1101111,
    JALR     = 0b1100111,
    BRANCH   = 0b1100011,
    LOAD     = 0b0000011,
    STORE    = 0b0100011,
    OP_IMM   = 0b0010011,
    OP       = 0b0110011,
    MISC_MEM = 0b0001111,
    SYSTEM   = 0b1110011,
};

// function to extract the opcode
// bits [6 - 0]
[[nodiscard]]
constexpr Word opcode(Word instruction) noexcept {
    return bits(instruction, 6, 0);
}

// function to extract the destination register
// bits [11 - 7]
[[nodiscard]]
constexpr RegIndex rd(Word instruction) noexcept {
    return static_cast<RegIndex>(bits(instruction, 11, 7));
}

// function to extract the funct3
// bits [14 - 12]
[[nodiscard]]
constexpr Word funct3(Word instruction) noexcept {
    return bits(instruction, 14, 12);
}

// function to extract the first source operand
// bits [19 - 15]
[[nodiscard]]
constexpr RegIndex rs1(Word instruction) noexcept {
    return static_cast<RegIndex>(bits(instruction, 19, 15));
}

// function to extract second source operand
// bits [24 - 20]
[[nodiscard]]
constexpr RegIndex rs2(Word instruction) noexcept {
    return static_cast<RegIndex>(bits(instruction, 24, 20));
}

// function to extract the funct7
// bits [31 - 25]
[[nodiscard]]
constexpr Word funct7(Word instruction) noexcept {
    return bits(instruction, 31, 25);
}

// function to extract the immidiate  
// instruction JALR, loads, OP-IMM
// bits [31 - 20]
[[nodiscard]]
constexpr Word imm_i(Word instruction) noexcept {
    return sign_extend(bits(instruction, 31, 20), 12);
}

// function to extract the immidiate  
// instruction stores
// bits [31 - 25] [11 - 7]
[[nodiscard]]
constexpr Word imm_s(Word instruction) noexcept {
    //imm[11:5]
    Word imm = bits(instruction, 31, 25);
    imm <<= 5;
    //imm[4:0]
    imm |= bits(instruction, 11, 7);
    return sign_extend(imm, 12);
}

// function to extract the immidiate  
// instruction branches
// bits [31 - 25] [11 - 7]
[[nodiscard]]
constexpr Word imm_b(Word instruction) noexcept {
    const Word imm = (bits(instruction, 31, 31) << 12)
                   | (bits(instruction, 7, 7) << 11)
                   | (bits(instruction, 30, 25) << 5)
                   | (bits(instruction, 11, 8) << 1);
    return sign_extend(imm, 13);
}

// function to extract the immidiate  
// instruction LUI, AUIPC
// bits [31 - 20]
[[nodiscard]]
constexpr Word imm_u(Word instruction) noexcept {
    return bits(instruction, 31, 12) << 12;
}

// function to extract the immidiate  
// instruction JAL
// bits [31 - 20]
[[nodiscard]]
constexpr Word imm_j(Word instruction) noexcept {
    const Word imm = (bits(instruction, 31, 31) << 20)
                   | (bits(instruction, 19, 12) << 12)
                   | (bits(instruction, 20, 20) << 11)
                   | (bits(instruction, 30, 21) << 1);
    return sign_extend(imm, 21);
}


}

#endif