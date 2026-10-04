#include <gtest/gtest.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <string_view>

#include "rvsim/register_file.hpp"
#include "rvsim/types.hpp"

using rvsim::RegisterFile;
using rvsim::regIndex;
using rvsim::Word;

// 3.1: the design, pinned down at compile time. If one is wrong, this file doesn't compile.
static_assert(RegisterFile::numRegisters == 32);
static_assert(std::default_initializable<RegisterFile>);
static_assert(std::equality_comparable<RegisterFile>);   // the defaulted operator==

// No operator[]: `regs[0] = 5` must not compile, because it would break x0. The concept asks
// "does this expression compile?", so the static_assert fails the build if one is ever added.
template <typename T>
concept HasSubscript = requires(T& regs) { regs[0]; };
static_assert(!HasSubscript<RegisterFile>);

namespace {

// A different value for every register, with bits set in every byte, so a write that lands
// in the wrong register (or only partly lands) can't go unnoticed.
Word
value_for(std::size_t i) {
    return static_cast<Word>(0x01010101u * i) ^ 0x80000000u;
}

}

// ---- T3.1: a new register file reads 0; x0 ignores writes; x1 to x31 round-trip ----

TEST(TestingRegisterFile, NewRegisterFileReadsZeroEverywhere) {
    const RegisterFile regs;

    for (std::size_t i = 0; i < RegisterFile::numRegisters; i++)
        EXPECT_EQ(regs.read(static_cast<regIndex>(i)), Word{0}) << "x" << i;
}

// The 3.1 example: after writing 5 to x0 and 12 to x10, x0 reads 0 and x10 reads 12.
TEST(TestingRegisterFile, WritesToX0AreIgnored) {
    RegisterFile regs;

    regs.write(0, 5);
    regs.write(10, 12);

    EXPECT_EQ(regs.read(0), Word{0});
    EXPECT_EQ(regs.read(10), Word{12});

    // Every bit pattern, not only small values.
    regs.write(0, 0xFFFFFFFF);
    EXPECT_EQ(regs.read(0), Word{0});
}

// Write every register first, then read them all back: each one holds its own value, so no
// two registers share storage.
TEST(TestingRegisterFile, X1ToX31RoundTrip) {
    RegisterFile regs;

    for (std::size_t i = 1; i < RegisterFile::numRegisters; i++)
        regs.write(static_cast<regIndex>(i), value_for(i));

    for (std::size_t i = 1; i < RegisterFile::numRegisters; i++)
        EXPECT_EQ(regs.read(static_cast<regIndex>(i)), value_for(i)) << "x" << i;
    EXPECT_EQ(regs.read(0), Word{0});
}

// All 32 bits survive, and a later write replaces the earlier one.
TEST(TestingRegisterFile, FullWordsAndOverwrites) {
    RegisterFile regs;

    regs.write(31, 0xFFFFFFFF);
    EXPECT_EQ(regs.read(31), Word{0xFFFFFFFF});
    regs.write(31, 0x80000000);
    EXPECT_EQ(regs.read(31), Word{0x80000000});
    regs.write(31, 0);
    EXPECT_EQ(regs.read(31), Word{0});
}

// The defaulted operator==, which 3.3 builds on.
TEST(TestingRegisterFile, EqualOnlyWhenEveryRegisterMatches) {
    RegisterFile a;
    RegisterFile b;
    EXPECT_EQ(a, b);

    a.write(17, 1);
    EXPECT_NE(a, b);
    b.write(17, 1);
    EXPECT_EQ(a, b);

    // A write to x0 is ignored, so it can't make two register files differ.
    a.write(0, 0xDEADBEEF);
    EXPECT_EQ(a, b);
}

// ---- T3.2: ABI names ----

// The T3.2 rows.
TEST(TestingAbiName, Examples) {
    EXPECT_EQ(rvsim::abi_name(0), "zero");
    EXPECT_EQ(rvsim::abi_name(1), "ra");
    EXPECT_EQ(rvsim::abi_name(2), "sp");
    EXPECT_EQ(rvsim::abi_name(8), "s0");   // also called fp, but objdump prints s0
    EXPECT_EQ(rvsim::abi_name(10), "a0");
    EXPECT_EQ(rvsim::abi_name(31), "t6");
}

// Beyond T3.2: all 32, in the order 3.2 lists them.
TEST(TestingAbiName, AllThirtyTwoInOrder) {
    const std::array<std::string_view, 32> expected = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0",
        "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5",
        "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
    };

    for (std::size_t i = 0; i < expected.size(); i++)
        EXPECT_EQ(rvsim::abi_name(static_cast<regIndex>(i)), expected[i]) << "x" << i;
}

// ---- Optional: death tests ----

// An index above 31 is a bug in the simulator, so read, write, and abi_name stop at their
// assert. As in test_bits.cpp, asserts are compiled out when NDEBUG is defined, so the test
// skips itself in release builds.
TEST(TestingRegisterFileDeathTest, IndexAbove31StopsAtTheAssert) {
#ifdef NDEBUG
    GTEST_SKIP() << "assert is compiled out when NDEBUG is defined";
#else
    RegisterFile regs;
    EXPECT_DEATH((void)regs.read(32), "index < numRegisters");
    EXPECT_DEATH(regs.write(32, 1), "index < numRegisters");
    EXPECT_DEATH((void)rvsim::abi_name(32), "index < RegisterFile::numRegisters");
#endif
}
