#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <ostream>
#include <set>
#include <string_view>
#include <type_traits>
#include <vector>

#include "rvsim/fields.hpp"
#include "rvsim/types.hpp"

#include "rv32i_vectors.hpp"

using rvsim::Opcode;
using rvsim::Word;
using rvsim::RegIndex;

// ---- 4.2: Opcode, at compile time ----

// The underlying type is Word, the type opcode() returns, so converting is exact both ways.
static_assert(std::is_same_v<std::underlying_type_t<Opcode>, Word>);

// Scoped: an Opcode doesn't quietly turn into a number. Comparing one with opcode()'s result
// takes a static_cast, on purpose.
static_assert(!std::is_convertible_v<Opcode, Word>);

// Every value, from the spec's listing table.
static_assert(static_cast<Word>(Opcode::LUI) == 0x37);
static_assert(static_cast<Word>(Opcode::AUIPC) == 0x17);
static_assert(static_cast<Word>(Opcode::JAL) == 0x6F);
static_assert(static_cast<Word>(Opcode::JALR) == 0x67);
static_assert(static_cast<Word>(Opcode::BRANCH) == 0x63);
static_assert(static_cast<Word>(Opcode::LOAD) == 0x03);
static_assert(static_cast<Word>(Opcode::STORE) == 0x23);
static_assert(static_cast<Word>(Opcode::OP_IMM) == 0x13);
static_assert(static_cast<Word>(Opcode::OP) == 0x33);
static_assert(static_cast<Word>(Opcode::MISC_MEM) == 0x0F);
static_assert(static_cast<Word>(Opcode::SYSTEM) == 0x73);

// ---- 4.3 and 4.4: return types, at compile time ----

// Register fields are RegIndex, the type RegisterFile::read and write take; the rest are Word.
static_assert(std::is_same_v<decltype(rvsim::opcode(0)), Word>);
static_assert(std::is_same_v<decltype(rvsim::rd(0)), RegIndex>);
static_assert(std::is_same_v<decltype(rvsim::funct3(0)), Word>);
static_assert(std::is_same_v<decltype(rvsim::rs1(0)), RegIndex>);
static_assert(std::is_same_v<decltype(rvsim::rs2(0)), RegIndex>);
static_assert(std::is_same_v<decltype(rvsim::funct7(0)), Word>);
static_assert(std::is_same_v<decltype(rvsim::imm_i(0)), Word>);
static_assert(std::is_same_v<decltype(rvsim::imm_s(0)), Word>);
static_assert(std::is_same_v<decltype(rvsim::imm_b(0)), Word>);
static_assert(std::is_same_v<decltype(rvsim::imm_u(0)), Word>);
static_assert(std::is_same_v<decltype(rvsim::imm_j(0)), Word>);

// T4.1 at compile time: one row per builder from the labs.md table. The builders are constexpr,
// so if one is wrong this file doesn't compile.
static_assert(rvsim::imm_i(0xFFF00513) == 0xFFFFFFFF);   // addi a0, zero, -1
static_assert(rvsim::imm_s(0xFEC12E23) == 0xFFFFFFFC);   // sw a2, -4(sp)
static_assert(rvsim::imm_b(0xFE051CE3) == 0xFFFFFFF8);   // bne a0, zero, -8
static_assert(rvsim::imm_u(0x12345737) == 0x12345000);   // lui a4, 0x12345
static_assert(rvsim::imm_j(0xFF1FF06F) == 0xFFFFFFF0);   // jal zero, -16

// Helper types and functions go in an unnamed namespace: a struct with the same name in another
// test file would otherwise be a second, different definition in the same program.
namespace {

// 1 << (n - lo) if bit n is inside the field hi..lo, else 0: what an extractor should return for
// a word with only bit n set.
Word
in_field(std::uint32_t n, std::uint32_t hi, std::uint32_t lo) {
    if (n < lo || n > hi)
        return 0;
    return Word{1} << (n - lo);
}

}

// ---- 4.3: field extractors ----

// The 4.3 example: add a2, a0, a1.
TEST(TestingFields, AddExample) {
    const Word add = 0x00B50633;

    EXPECT_EQ(rvsim::opcode(add), Word{0x33});
    EXPECT_EQ(rvsim::rd(add), 12);
    EXPECT_EQ(rvsim::funct3(add), Word{0});
    EXPECT_EQ(rvsim::rs1(add), 10);
    EXPECT_EQ(rvsim::rs2(add), 11);
    EXPECT_EQ(rvsim::funct7(add), Word{0});
}

// sub a3, a1, a0: funct7 0x20 is the only thing that makes it a SUB and not an ADD.
TEST(TestingFields, SubExample) {
    const Word sub = 0x40A586B3;

    EXPECT_EQ(rvsim::opcode(sub), Word{0x33});
    EXPECT_EQ(rvsim::rd(sub), 13);
    EXPECT_EQ(rvsim::funct3(sub), Word{0});
    EXPECT_EQ(rvsim::rs1(sub), 11);
    EXPECT_EQ(rvsim::rs2(sub), 10);
    EXPECT_EQ(rvsim::funct7(sub), Word{0x20});
}

// addi a0, zero, -1 has no rs2 and no funct7, but the extractors don't know that: they return
// whatever sits in those bits, here the top of the immediate.
TEST(TestingFields, ExtractorsDontCheckTheFormat) {
    const Word addi = 0xFFF00513;

    EXPECT_EQ(rvsim::opcode(addi), Word{0x13});
    EXPECT_EQ(rvsim::rd(addi), 10);
    EXPECT_EQ(rvsim::rs1(addi), 0);
    EXPECT_EQ(rvsim::rs2(addi), 31);
    EXPECT_EQ(rvsim::funct7(addi), Word{0x7F});
}

// Beyond T4.3: on the all-ones word every extractor returns its field's largest value, so a
// field cut one bit too narrow shows up.
TEST(TestingFields, AllOnesWordGivesFullFields) {
    const Word ones = 0xFFFFFFFF;

    EXPECT_EQ(rvsim::opcode(ones), Word{0x7F});
    EXPECT_EQ(rvsim::rd(ones), 31);
    EXPECT_EQ(rvsim::funct3(ones), Word{7});
    EXPECT_EQ(rvsim::rs1(ones), 31);
    EXPECT_EQ(rvsim::rs2(ones), 31);
    EXPECT_EQ(rvsim::funct7(ones), Word{0x7F});
}

// Beyond T4.3: one-bit sweep. With only bit n set, exactly the field that contains bit n sees
// it, at position n - lo. A field cut at the wrong place, or one bit too wide, shows up here.
TEST(TestingFields, EachBitLandsInExactlyOneField) {
    for (std::uint32_t n = 0; n < 32; n++) {
        const Word word = Word{1} << n;
        SCOPED_TRACE(std::format("only bit {} set: 0x{:08x}", n, word));

        EXPECT_EQ(rvsim::opcode(word), in_field(n, 6, 0));
        EXPECT_EQ(Word{rvsim::rd(word)}, in_field(n, 11, 7));
        EXPECT_EQ(rvsim::funct3(word), in_field(n, 14, 12));
        EXPECT_EQ(Word{rvsim::rs1(word)}, in_field(n, 19, 15));
        EXPECT_EQ(Word{rvsim::rs2(word)}, in_field(n, 24, 20));
        EXPECT_EQ(rvsim::funct7(word), in_field(n, 31, 25));
    }
}

// ---- T4.1: every immediate builder at zero, small positive, small negative, and both limits ----
//
// One parameterized suite for all five builders: each row carries a pointer to the builder it
// tests. Every word comes from the cross assembler. The labs.md table is the first rows; the
// zero rows and two small values were added.

namespace {

struct ImmCase {
    std::string_view builder;          // the builder's name, for messages and test names
    Word (*build)(Word) noexcept;      // the builder itself
    Word word;
    Word expected;
};

// GoogleTest finds PrintTo next to the type, so it lives in the same unnamed namespace.
void PrintTo(const ImmCase& c, std::ostream* os) {
    *os << std::format("{}(0x{:08x})", c.builder, c.word);
}

}

class TestingImmediates : public ::testing::TestWithParam<ImmCase> {};

TEST_P(TestingImmediates, BuildsTheImmediate) {
    const ImmCase& c = GetParam();
    EXPECT_EQ(c.build(c.word), c.expected);
}

const ImmCase immCases[] = {
    // I-type, -2048 to 2047
    {"imm_i", rvsim::imm_i, 0xFFF00513, 0xFFFFFFFF},   // addi a0, zero, -1
    {"imm_i", rvsim::imm_i, 0x7FF00513, 0x000007FF},   // addi a0, zero, 2047: largest
    {"imm_i", rvsim::imm_i, 0x80000513, 0xFFFFF800},   // addi a0, zero, -2048: most negative
    {"imm_i", rvsim::imm_i, 0x00000513, 0x00000000},   // addi a0, zero, 0
    {"imm_i", rvsim::imm_i, 0x00500513, 0x00000005},   // addi a0, zero, 5: Lab 0's first word
    // S-type, -2048 to 2047, in two pieces
    {"imm_s", rvsim::imm_s, 0xFEC12E23, 0xFFFFFFFC},   // sw a2, -4(sp)
    {"imm_s", rvsim::imm_s, 0x7EC12FA3, 0x000007FF},   // sw a2, 2047(sp): largest
    {"imm_s", rvsim::imm_s, 0x80C12023, 0xFFFFF800},   // sw a2, -2048(sp): most negative
    {"imm_s", rvsim::imm_s, 0x00C12023, 0x00000000},   // sw a2, 0(sp)
    {"imm_s", rvsim::imm_s, 0x00C12423, 0x00000008},   // sw a2, 8(sp)
    // B-type, -4096 to 4094, bit 0 always 0
    {"imm_b", rvsim::imm_b, 0x00B50463, 0x00000008},   // beq a0, a1, +8
    {"imm_b", rvsim::imm_b, 0xFE051CE3, 0xFFFFFFF8},   // bne a0, zero, -8
    {"imm_b", rvsim::imm_b, 0x7EB50FE3, 0x00000FFE},   // beq a0, a1, +4094: largest
    {"imm_b", rvsim::imm_b, 0x80B50063, 0xFFFFF000},   // beq a0, a1, -4096: most negative
    {"imm_b", rvsim::imm_b, 0x00B50063, 0x00000000},   // beq a0, a1, +0
    // U-type: upper 20 bits in place, low 12 bits zero
    {"imm_u", rvsim::imm_u, 0x12345737, 0x12345000},   // lui a4, 0x12345
    {"imm_u", rvsim::imm_u, 0xFFFFF737, 0xFFFFF000},   // lui a4, 0xfffff: -4096
    {"imm_u", rvsim::imm_u, 0x7FFFF737, 0x7FFFF000},   // lui a4, 0x7ffff: largest
    {"imm_u", rvsim::imm_u, 0x80000737, 0x80000000},   // lui a4, 0x80000: most negative
    {"imm_u", rvsim::imm_u, 0x00000737, 0x00000000},   // lui a4, 0x0
    // J-type, -1048576 to 1048574, bit 0 always 0
    {"imm_j", rvsim::imm_j, 0x010000EF, 0x00000010},   // jal ra, +16
    {"imm_j", rvsim::imm_j, 0xFF1FF06F, 0xFFFFFFF0},   // jal zero, -16
    {"imm_j", rvsim::imm_j, 0x7FFFF0EF, 0x000FFFFE},   // jal ra, +1048574: largest
    {"imm_j", rvsim::imm_j, 0x8000006F, 0xFFF00000},   // jal zero, -1048576: most negative
    {"imm_j", rvsim::imm_j, 0x000000EF, 0x00000000},   // jal ra, +0
};

INSTANTIATE_TEST_SUITE_P(Table, TestingImmediates, ::testing::ValuesIn(immCases));

// ---- T4.2: B and J immediates always have bit 0 clear ----

// imm[0] isn't stored in a branch or a jal, so no word can set it: not a backward offset, not
// the all-ones word, and not a word with bit 7 or bit 20 set (where S and I keep their imm[0]).
// Checked on every vector, whatever its format, plus those three words.
TEST(TestingOffsets, BitZeroIsAlwaysClear) {
    std::vector<Word> words = {0xFFFFFFFF, 0x00000080, 0x00100000};
    for (const TestVector& v : rv32iVectors)
        words.push_back(v.word);

    for (const Word word : words) {
        SCOPED_TRACE(std::format("word 0x{:08x}", word));
        EXPECT_EQ(rvsim::imm_b(word) & 1u, Word{0});
        EXPECT_EQ(rvsim::imm_j(word) & 1u, Word{0});
    }
}

// Backward offsets: the labs.md examples and both most-negative limits are negative and even.
TEST(TestingOffsets, BackwardOffsets) {
    EXPECT_EQ(rvsim::imm_b(0xFE051CE3), Word{0xFFFFFFF8});   // bne a0, zero, -8
    EXPECT_EQ(rvsim::imm_b(0x80B50063), Word{0xFFFFF000});   // -4096
    EXPECT_EQ(rvsim::imm_j(0xFF1FF06F), Word{0xFFFFFFF0});   // jal zero, -16
    EXPECT_EQ(rvsim::imm_j(0x8000006F), Word{0xFFF00000});   // -1048576
}

// All ones: every stored immediate bit is 1, so B and J give -2 (bit 0 is the one 0), while I
// and S give -1 and U gives 0xFFFFF000.
TEST(TestingOffsets, AllOnesWord) {
    EXPECT_EQ(rvsim::imm_b(0xFFFFFFFF), Word{0xFFFFFFFE});
    EXPECT_EQ(rvsim::imm_j(0xFFFFFFFF), Word{0xFFFFFFFE});
    EXPECT_EQ(rvsim::imm_i(0xFFFFFFFF), Word{0xFFFFFFFF});
    EXPECT_EQ(rvsim::imm_s(0xFFFFFFFF), Word{0xFFFFFFFF});
    EXPECT_EQ(rvsim::imm_u(0xFFFFFFFF), Word{0xFFFFF000});
}

// ---- Beyond T4.2: one-bit sweep over every immediate builder ----
//
// The spec's figures as data: instruction bits hi..lo hold immediate bits immLo and up. With only
// instruction bit n set, a builder must return that one immediate bit, or 0 if no piece covers
// bit n. Bit 31 is the sign: it becomes immediate bit signBit and is copied into every bit above
// it. A piece cut at the wrong place, shifted by the wrong amount, or extended from the wrong
// width fails here even if every vector happens to pass.

namespace {

struct Piece {
    std::uint32_t hi;
    std::uint32_t lo;
    std::uint32_t immLo;
};

struct ImmLayout {
    std::string_view builder;
    Word (*build)(Word) noexcept;
    std::vector<Piece> pieces;    // every piece except the sign bit
    std::uint32_t signBit;        // the immediate bit instruction bit 31 holds
};

}

TEST(TestingImmediateBits, EachBitLandsWhereTheSpecSays) {
    const ImmLayout layouts[] = {
        {"imm_i", rvsim::imm_i, {{30, 20, 0}}, 11},
        {"imm_s", rvsim::imm_s, {{11, 7, 0}, {30, 25, 5}}, 11},
        {"imm_b", rvsim::imm_b, {{11, 8, 1}, {30, 25, 5}, {7, 7, 11}}, 12},
        {"imm_u", rvsim::imm_u, {{30, 12, 12}}, 31},
        {"imm_j", rvsim::imm_j, {{30, 21, 1}, {20, 20, 11}, {19, 12, 12}}, 20},
    };

    for (const ImmLayout& layout : layouts) {
        for (std::uint32_t n = 0; n < 32; n++) {
            Word expected = 0;
            if (n == 31) {
                expected = ~Word{0} << layout.signBit;
            } else {
                for (const Piece& piece : layout.pieces) {
                    if (n >= piece.lo && n <= piece.hi)
                        expected = Word{1} << (piece.immLo + (n - piece.lo));
                }
            }

            const Word word = Word{1} << n;
            EXPECT_EQ(layout.build(word), expected)
                << std::format("{}(0x{:08x}), only bit {} set", layout.builder, word, n);
        }
    }
}

// ---- T4.3: every 4.5 vector gives the fields and immediate the disassembly shows ----

class TestingVectors : public ::testing::TestWithParam<TestVector> {};

TEST_P(TestingVectors, MatchesObjdump) {
    const TestVector& v = GetParam();
    const Word word = v.word;

    EXPECT_EQ(rvsim::opcode(word), v.opcode);

    // Only the fields this format has. In the other positions the word holds immediate bits.
    switch (v.format) {
        case Format::R:
            EXPECT_EQ(rvsim::rd(word), v.rd);
            EXPECT_EQ(rvsim::funct3(word), v.funct3);
            EXPECT_EQ(rvsim::rs1(word), v.rs1);
            EXPECT_EQ(rvsim::rs2(word), v.rs2);
            EXPECT_EQ(rvsim::funct7(word), v.funct7);
            break;
        case Format::I:
            EXPECT_EQ(rvsim::rd(word), v.rd);
            EXPECT_EQ(rvsim::funct3(word), v.funct3);
            EXPECT_EQ(rvsim::rs1(word), v.rs1);
            EXPECT_EQ(rvsim::imm_i(word), v.imm);
            break;
        case Format::S:
            EXPECT_EQ(rvsim::funct3(word), v.funct3);
            EXPECT_EQ(rvsim::rs1(word), v.rs1);
            EXPECT_EQ(rvsim::rs2(word), v.rs2);
            EXPECT_EQ(rvsim::imm_s(word), v.imm);
            break;
        case Format::B:
            EXPECT_EQ(rvsim::funct3(word), v.funct3);
            EXPECT_EQ(rvsim::rs1(word), v.rs1);
            EXPECT_EQ(rvsim::rs2(word), v.rs2);
            EXPECT_EQ(rvsim::imm_b(word), v.imm);
            break;
        case Format::U:
            EXPECT_EQ(rvsim::rd(word), v.rd);
            EXPECT_EQ(rvsim::imm_u(word), v.imm);
            break;
        case Format::J:
            EXPECT_EQ(rvsim::rd(word), v.rd);
            EXPECT_EQ(rvsim::imm_j(word), v.imm);
            break;
    }
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingVectors, ::testing::ValuesIn(rv32iVectors));

// ---- Beyond T4.3: the table itself ----

// vectors.S covers all 40 RV32I instructions: the mnemonic is the text up to the first space.
TEST(TestingVectorTable, CoversEveryRv32iInstruction) {
    const std::set<std::string_view> rv32i = {
        "lui", "auipc", "jal", "jalr",
        "beq", "bne", "blt", "bge", "bltu", "bgeu",
        "lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw",
        "addi", "slti", "sltiu", "xori", "ori", "andi", "slli", "srli", "srai",
        "add", "sub", "sll", "slt", "sltu", "xor", "srl", "sra", "or", "and",
        "fence", "ecall", "ebreak",
    };
    ASSERT_EQ(rv32i.size(), 40u);

    std::set<std::string_view> covered;
    for (const TestVector& v : rv32iVectors)
        covered.insert(v.text.substr(0, v.text.find(' ')));

    EXPECT_EQ(covered, rv32i);
}

// Rows are in address order, 4 bytes apart from 0, so no objdump line was skipped or copied
// twice.
TEST(TestingVectorTable, RowsAreFourBytesApart) {
    rvsim::Addr expected = 0;
    for (const TestVector& v : rv32iVectors) {
        EXPECT_EQ(v.pc, expected) << v.text;
        expected += 4;
    }
}
