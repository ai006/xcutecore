#include <gtest/gtest.h>

#include <concepts>
#include <cstddef>
#include <sstream>
#include <string>

#include "rvsim/arch_state.hpp"
#include "rvsim/register_file.hpp"
#include "rvsim/types.hpp"

using rvsim::Addr;
using rvsim::ArchState;
using rvsim::RegisterFile;
using rvsim::regIndex;
using rvsim::Word;

// 3.3: an ArchState can be made, copied as a snapshot, and compared.
static_assert(std::default_initializable<ArchState>);
static_assert(std::copyable<ArchState>);
static_assert(std::equality_comparable<ArchState>);

namespace {

// The state from the 3.3 example: pc 0x80000008, sp (x2) 0x800FFFF0, a2 (x12) 12.
ArchState
example_state() {
    ArchState state;
    state.pc = 0x80000008;
    state.regs.write(2, 0x800FFFF0);
    state.regs.write(12, 12);
    return state;
}

// What operator<< prints, as a string.
std::string
dump(const ArchState& state) {
    std::ostringstream out;
    out << state;
    return out.str();
}

}

// ---- 3.3: a new state starts at 0 ----

TEST(TestingArchState, NewStateIsAllZero) {
    const ArchState state;

    EXPECT_EQ(state.pc, Addr{0});
    for (std::size_t i = 0; i < RegisterFile::numRegisters; i++)
        EXPECT_EQ(state.regs.read(static_cast<regIndex>(i)), Word{0}) << "x" << i;
}

// ---- T3.3: states that differ only in the pc, or in one register, are unequal ----

TEST(TestingArchState, IdenticalStatesAreEqual) {
    EXPECT_EQ(ArchState{}, ArchState{});
    EXPECT_EQ(example_state(), example_state());

    // A copy is a snapshot: equal to the original until one of them changes.
    ArchState copy = example_state();
    EXPECT_EQ(copy, example_state());
    copy.regs.write(5, 1);
    EXPECT_NE(copy, example_state());
}

TEST(TestingArchState, DifferOnlyInThePc) {
    ArchState a = example_state();
    ArchState b = example_state();

    b.pc = 0x8000000C;
    EXPECT_NE(a, b);
    a.pc = 0x8000000C;
    EXPECT_EQ(a, b);
}

// Every register from x1 to x31, one at a time: a change to any single one makes the states
// unequal, so operator== can't be skipping any of them.
TEST(TestingArchState, DifferOnlyInOneRegister) {
    const ArchState original = example_state();

    for (std::size_t i = 1; i < RegisterFile::numRegisters; i++) {
        const regIndex index = static_cast<regIndex>(i);
        ArchState changed = original;
        changed.regs.write(index, changed.regs.read(index) ^ 1);
        EXPECT_NE(changed, original) << "only x" << i << " differs";
    }
}

// x0 can't be written, so a write to it can't make two states differ.
TEST(TestingArchState, WriteToX0DoesNotChangeTheState) {
    ArchState changed = example_state();
    changed.regs.write(0, 0xFFFFFFFF);

    EXPECT_EQ(changed, example_state());
}

// ---- 3.3 and "done when": the state dump prints all 32 registers as a readable table ----

// The 3.3 example in full: its first lines are the ones labs.md shows.
TEST(TestingArchStateDump, ExampleStatePrintsAsATable) {
    EXPECT_EQ(dump(example_state()),
              "pc  80000008\n"
              "x0  zero 00000000   x1  ra   00000000   x2  sp   800ffff0   x3  gp   00000000\n"
              "x4  tp   00000000   x5  t0   00000000   x6  t1   00000000   x7  t2   00000000\n"
              "x8  s0   00000000   x9  s1   00000000   x10 a0   00000000   x11 a1   00000000\n"
              "x12 a2   0000000c   x13 a3   00000000   x14 a4   00000000   x15 a5   00000000\n"
              "x16 a6   00000000   x17 a7   00000000   x18 s2   00000000   x19 s3   00000000\n"
              "x20 s4   00000000   x21 s5   00000000   x22 s6   00000000   x23 s7   00000000\n"
              "x24 s8   00000000   x25 s9   00000000   x26 s10  00000000   x27 s11  00000000\n"
              "x28 t3   00000000   x29 t4   00000000   x30 t5   00000000   x31 t6   00000000\n");
}

// Values print as 8 lowercase hex digits, zero-padded, including the largest one.
TEST(TestingArchStateDump, ValuesAreEightLowercaseHexDigits) {
    ArchState state;
    state.pc = 0xABCDEF00;
    state.regs.write(31, 0xFFFFFFFF);
    state.regs.write(30, 0x1);

    const std::string text = dump(state);
    EXPECT_NE(text.find("pc  abcdef00\n"), std::string::npos);
    EXPECT_NE(text.find("x30 t5   00000001   x31 t6   ffffffff\n"), std::string::npos);
}

// The dump leaves the stream's formatting alone: a number printed after it is still decimal.
// (std::hex is sticky, so a dump that set it would turn every later number into hex.)
TEST(TestingArchStateDump, LeavesTheStreamInDecimal) {
    std::ostringstream out;
    out << example_state() << 255;

    const std::string text = out.str();
    EXPECT_EQ(text.substr(text.size() - 4), "\n255");
}

// operator<< returns the stream, so output can be chained after it.
TEST(TestingArchStateDump, CanBeChained) {
    std::ostringstream out;
    out << "before\n" << ArchState{} << "after\n";

    const std::string text = out.str();
    EXPECT_EQ(text.substr(0, 7), "before\n");
    EXPECT_EQ(text.substr(text.size() - 6), "after\n");
}
