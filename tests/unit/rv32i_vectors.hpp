#ifndef RV32I_VECTORS_HPP
#define RV32I_VECTORS_HPP

#include <algorithm>
#include <format>
#include <ostream>
#include <string>
#include <string_view>

#include "rvsim/types.hpp"

// 4.5: every RV32I instruction, as the cross assembler encoded it and objdump decoded it.
//
// The source is tests/programs/lab4/vectors.S. Each row is one line of
//   riscv64-unknown-elf-objdump -d -M no-aliases build/lab4/vectors.o
// copied by hand: the expected values come from the toolchain, not from my own reading of the
// formats, so a misunderstanding in fields.hpp can't sneak into the expectations too.
//
// It's a header so that more than one test file can use it: fields_test.cpp checks the fields
// and immediates (T4.3), and Lab 5's decode tests check the decoded class and the disassembly
// (T5.1, T5.3) against the same rows. The table is `inline constexpr`: constexpr so it's built
// at compile time with no static initialization order to worry about, and inline so every file
// that includes it shares one table instead of getting its own copy.

// The six instruction formats (4.1). A row's format says which fields and which immediate
// builder apply to it.
enum class Format { R, I, S, B, U, J };

// One instruction. Fields the format doesn't have hold 0 here and aren't checked: in those
// bit positions the word holds immediate bits, or nothing.
//
// pc is the address objdump printed. vectors.o isn't linked, so the first row is at 0. text is
// objdump's text with one space after the mnemonic; branch and jump targets in it are absolute
// addresses. imm is what the format's builder returns, sign-extended; for B and J it's the
// target minus pc. R-type rows have no immediate and hold 0.
struct TestVector {
    rvsim::Addr pc;
    rvsim::Word word;
    std::string_view text;
    Format format;
    rvsim::Word opcode;
    rvsim::RegIndex rd;
    rvsim::RegIndex rs1;
    rvsim::RegIndex rs2;
    rvsim::Word funct3;
    rvsim::Word funct7;
    rvsim::Word imm;
};

// How GoogleTest prints a row: in failure messages, and in the CTest name of each table row.
// Without it, both show a raw byte dump of the struct. CTest names drop spaces, so spaces become
// underscores: "beq a0,a1,4c" at 0x44 prints as 044_beq_a0,a1,4c. `inline` because it's
// defined in a header.
inline void PrintTo(const TestVector& v, std::ostream* os) {
    std::string text{v.text};
    std::ranges::replace(text, ' ', '_');
    *os << std::format("{:03x}_{}", v.pc, text);
}

// Every row lists every member, in order. Leaving one out fails the build with "missing
// initializer for member" (-Wmissing-field-initializers, from -Wextra).
inline constexpr TestVector rv32iVectors[] = {
    // pc    word        text                     format     opc   rd rs1 rs2 f3  f7    imm
    // U-type: LUI, AUIPC
    {0x000, 0x12345737, "lui a4,0x12345",        Format::U, 0x37, 14,  0,  0, 0, 0x00, 0x12345000},
    {0x004, 0x00000037, "lui zero,0x0",          Format::U, 0x37,  0,  0,  0, 0, 0x00, 0x00000000},
    {0x008, 0x00001FB7, "lui t6,0x1",            Format::U, 0x37, 31,  0,  0, 0, 0x00, 0x00001000},
    {0x00C, 0xFFFFF537, "lui a0,0xfffff",        Format::U, 0x37, 10,  0,  0, 0, 0x00, 0xFFFFF000},
    {0x010, 0x7FFFF437, "lui s0,0x7ffff",        Format::U, 0x37,  8,  0,  0, 0, 0x00, 0x7FFFF000},
    {0x014, 0x800004B7, "lui s1,0x80000",        Format::U, 0x37,  9,  0,  0, 0, 0x00, 0x80000000},
    {0x018, 0x00000297, "auipc t0,0x0",          Format::U, 0x17,  5,  0,  0, 0, 0x00, 0x00000000},
    {0x01C, 0xAAAAAF97, "auipc t6,0xaaaaa",      Format::U, 0x17, 31,  0,  0, 0, 0x00, 0xAAAAA000},
    {0x020, 0x55555097, "auipc ra,0x55555",      Format::U, 0x17,  1,  0,  0, 0, 0x00, 0x55555000},

    // J-type: JAL. imm is target - pc
    {0x024, 0x010000EF, "jal ra,34",             Format::J, 0x6F,  1,  0,  0, 0, 0x00, 0x00000010},
    {0x028, 0xFF1FF06F, "jal zero,18",           Format::J, 0x6F,  0,  0,  0, 0, 0x00, 0xFFFFFFF0},
    {0x02C, 0x00000FEF, "jal t6,2c",             Format::J, 0x6F, 31,  0,  0, 0, 0x00, 0x00000000},
    {0x030, 0x0020056F, "jal a0,32",             Format::J, 0x6F, 10,  0,  0, 0, 0x00, 0x00000002},
    {0x034, 0x7FFFF06F, "jal zero,100032",       Format::J, 0x6F,  0,  0,  0, 0, 0x00, 0x000FFFFE},
    {0x038, 0x800000EF, "jal ra,fff00038",       Format::J, 0x6F,  1,  0,  0, 0, 0x00, 0xFFF00000},
    {0x03C, 0x2ABAA96F, "jal s2,aaae6",          Format::J, 0x6F, 18,  0,  0, 0, 0x00, 0x000AAAAA},
    {0x040, 0xD54559EF, "jal s3,fff55594",       Format::J, 0x6F, 19,  0,  0, 0, 0x00, 0xFFF55554},

    // B-type: BEQ, BNE, BLT, BGE, BLTU, BGEU. imm is target - pc
    {0x044, 0x00B50463, "beq a0,a1,4c",          Format::B, 0x63,  0, 10, 11, 0, 0x00, 0x00000008},
    {0x048, 0xFE051CE3, "bne a0,zero,40",        Format::B, 0x63,  0, 10,  0, 1, 0x00, 0xFFFFFFF8},
    {0x04C, 0x01F00063, "beq zero,t6,4c",        Format::B, 0x63,  0,  0, 31, 0, 0x00, 0x00000000},
    {0x050, 0x000F9163, "bne t6,zero,52",        Format::B, 0x63,  0, 31,  0, 1, 0x00, 0x00000002},
    {0x054, 0x7FDF4FE3, "blt t5,t4,1052",        Format::B, 0x63,  0, 30, 29, 4, 0x00, 0x00000FFE},
    {0x058, 0x815A5063, "bge s4,s5,fffff058",    Format::B, 0x63,  0, 20, 21, 5, 0x00, 0xFFFFF000},
    {0x05C, 0x2AD665E3, "bltu a2,a3,b06",        Format::B, 0x63,  0, 12, 13, 6, 0x00, 0x00000AAA},
    {0x060, 0xD4F77A63, "bgeu a4,a5,fffff5b4",   Format::B, 0x63,  0, 14, 15, 7, 0x00, 0xFFFFF554},

    // S-type: SB, SH, SW
    {0x064, 0xFEC12E23, "sw a2,-4(sp)",          Format::S, 0x23,  0,  2, 12, 2, 0x00, 0xFFFFFFFC},
    {0x068, 0x01F00023, "sb t6,0(zero)",         Format::S, 0x23,  0,  0, 31, 0, 0x00, 0x00000000},
    {0x06C, 0x000F9423, "sh zero,8(t6)",         Format::S, 0x23,  0, 31,  0, 1, 0x00, 0x00000008},
    {0x070, 0x7E112FA3, "sw ra,2047(sp)",        Format::S, 0x23,  0,  2,  1, 2, 0x00, 0x000007FF},
    {0x074, 0x816B8023, "sb s6,-2048(s7)",       Format::S, 0x23,  0, 23, 22, 0, 0x00, 0xFFFFF800},
    {0x078, 0x558C9AA3, "sh s8,1365(s9)",        Format::S, 0x23,  0, 25, 24, 1, 0x00, 0x00000555},
    {0x07C, 0xABADA523, "sw s10,-1366(s11)",     Format::S, 0x23,  0, 27, 26, 2, 0x00, 0xFFFFFAAA},

    // I-type loads: LB, LH, LW, LBU, LHU
    {0x080, 0x00058503, "lb a0,0(a1)",           Format::I, 0x03, 10, 11,  0, 0, 0x00, 0x00000000},
    {0x084, 0x00401F83, "lh t6,4(zero)",         Format::I, 0x03, 31,  0,  0, 1, 0x00, 0x00000004},
    {0x088, 0xFFCFA003, "lw zero,-4(t6)",        Format::I, 0x03,  0, 31,  0, 2, 0x00, 0xFFFFFFFC},
    {0x08C, 0x7FF3C303, "lbu t1,2047(t2)",       Format::I, 0x03,  6,  7,  0, 4, 0x00, 0x000007FF},
    {0x090, 0x80045E03, "lhu t3,-2048(s0)",      Format::I, 0x03, 28,  8,  0, 5, 0x00, 0xFFFFF800},

    // I-type: JALR
    {0x094, 0x00008067, "jalr zero,0(ra)",       Format::I, 0x67,  0,  1,  0, 0, 0x00, 0x00000000},
    {0x098, 0x800F80E7, "jalr ra,-2048(t6)",     Format::I, 0x67,  1, 31,  0, 0, 0x00, 0xFFFFF800},
    {0x09C, 0x7FF00FE7, "jalr t6,2047(zero)",    Format::I, 0x67, 31,  0,  0, 0, 0x00, 0x000007FF},

    // I-type OP-IMM. For the shifts, imm is what imm_i returns: the shift amount objdump
    // prints, plus 0x400 for SRAI
    {0x0A0, 0xFFF00513, "addi a0,zero,-1",       Format::I, 0x13, 10,  0,  0, 0, 0x00, 0xFFFFFFFF},
    {0x0A4, 0x00000013, "addi zero,zero,0",      Format::I, 0x13,  0,  0,  0, 0, 0x00, 0x00000000},
    {0x0A8, 0x01010113, "addi sp,sp,16",         Format::I, 0x13,  2,  2,  0, 0, 0x00, 0x00000010},
    {0x0AC, 0x7FF62593, "slti a1,a2,2047",       Format::I, 0x13, 11, 12,  0, 2, 0x00, 0x000007FF},
    {0x0B0, 0x80073693, "sltiu a3,a4,-2048",     Format::I, 0x13, 13, 14,  0, 3, 0x00, 0xFFFFF800},
    {0x0B4, 0xFFFFCF93, "xori t6,t6,-1",         Format::I, 0x13, 31, 31,  0, 4, 0x00, 0xFFFFFFFF},
    {0x0B8, 0x5554E413, "ori s0,s1,1365",        Format::I, 0x13,  8,  9,  0, 6, 0x00, 0x00000555},
    {0x0BC, 0xAAA87793, "andi a5,a6,-1366",      Format::I, 0x13, 15, 16,  0, 7, 0x00, 0xFFFFFAAA},
    {0x0C0, 0x01F01F93, "slli t6,zero,0x1f",     Format::I, 0x13, 31,  0,  0, 1, 0x00, 0x0000001F},
    {0x0C4, 0x0015D513, "srli a0,a1,0x1",        Format::I, 0x13, 10, 11,  0, 5, 0x00, 0x00000001},
    {0x0C8, 0x41F6D613, "srai a2,a3,0x1f",       Format::I, 0x13, 12, 13,  0, 5, 0x00, 0x0000041F},
    {0x0CC, 0x400FD013, "srai zero,t6,0x0",      Format::I, 0x13,  0, 31,  0, 5, 0x00, 0x00000400},

    // R-type OP: no immediate
    {0x0D0, 0x00B50633, "add a2,a0,a1",          Format::R, 0x33, 12, 10, 11, 0, 0x00, 0x00000000},
    {0x0D4, 0x40A586B3, "sub a3,a1,a0",          Format::R, 0x33, 13, 11, 10, 0, 0x20, 0x00000000},
    {0x0D8, 0x01F01FB3, "sll t6,zero,t6",        Format::R, 0x33, 31,  0, 31, 1, 0x00, 0x00000000},
    {0x0DC, 0x000FA033, "slt zero,t6,zero",      Format::R, 0x33,  0, 31,  0, 2, 0x00, 0x00000000},
    {0x0E0, 0x0124B433, "sltu s0,s1,s2",         Format::R, 0x33,  8,  9, 18, 3, 0x00, 0x00000000},
    {0x0E4, 0x0107C733, "xor a4,a5,a6",          Format::R, 0x33, 14, 15, 16, 4, 0x00, 0x00000000},
    {0x0E8, 0x007352B3, "srl t0,t1,t2",          Format::R, 0x33,  5,  6,  7, 5, 0x00, 0x00000000},
    {0x0EC, 0x41EEDE33, "sra t3,t4,t5",          Format::R, 0x33, 28, 29, 30, 5, 0x20, 0x00000000},
    {0x0F0, 0x001DED33, "or s10,s11,ra",         Format::R, 0x33, 26, 27,  1, 6, 0x00, 0x00000000},
    {0x0F4, 0x002271B3, "and gp,tp,sp",          Format::R, 0x33,  3,  4,  2, 7, 0x00, 0x00000000},

    // MISC-MEM: FENCE. imm holds pred (bits 7:4) and succ (bits 3:0)
    {0x0F8, 0x0FF0000F, "fence iorw,iorw",       Format::I, 0x0F,  0,  0,  0, 0, 0x00, 0x000000FF},
    {0x0FC, 0x0210000F, "fence r,w",             Format::I, 0x0F,  0,  0,  0, 0, 0x00, 0x00000021},

    // SYSTEM: ECALL, EBREAK. Only imm tells them apart
    {0x100, 0x00000073, "ecall",                 Format::I, 0x73,  0,  0,  0, 0, 0x00, 0x00000000},
    {0x104, 0x00100073, "ebreak",                Format::I, 0x73,  0,  0,  0, 0, 0x00, 0x00000001},
};

#endif
