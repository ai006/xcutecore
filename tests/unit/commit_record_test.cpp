#include <gtest/gtest.h>

#include <concepts>
#include <cstddef>
#include <optional>
#include <type_traits>

#include "rvsim/commit_record.hpp"
#include "rvsim/types.hpp"

using rvsim::Addr;
using rvsim::CommitRecord;
using rvsim::MemWrite;
using rvsim::RegWrite;
using rvsim::Word;

// 3.4: plain aggregates (so designated initializers work), each with a defaulted operator==.
static_assert(std::is_aggregate_v<RegWrite>);
static_assert(std::is_aggregate_v<MemWrite>);
static_assert(std::is_aggregate_v<CommitRecord>);
static_assert(std::equality_comparable<RegWrite>);
static_assert(std::equality_comparable<MemWrite>);
static_assert(std::equality_comparable<CommitRecord>);

namespace {

// The two 3.4 examples. Every member is named, including the empty optional: GCC 13 with
// -Wextra rejects a designated initializer that leaves one out (-Wmissing-field-initializers).

// sub a3, a1, a0 at 0x8000000C with a0 = 5 and a1 = 7: x13 = 2, no memory write.
CommitRecord
sub_record() {
    return CommitRecord{
        .pc = 0x8000000C,
        .word = 0x40A586B3,
        .regWrite = RegWrite{.rd = 13, .value = 2},
        .memWrite = std::nullopt,
    };
}

// sw a2, -4(sp) at 0x80000010 with sp = 0x800FFFF0 and a2 = 12: no register write, and 12
// written 4 bytes wide to 0x800FFFEC.
CommitRecord
sw_record() {
    return CommitRecord{
        .pc = 0x80000010,
        .word = 0xFEC12E23,
        .regWrite = std::nullopt,
        .memWrite = MemWrite{.addr = 0x800FFFEC, .width = 4, .value = 12},
    };
}

}

// ---- 3.4: the two examples hold what they should ----

TEST(TestingCommitRecord, SubExample) {
    const CommitRecord record = sub_record();

    EXPECT_EQ(record.pc, Addr{0x8000000C});
    EXPECT_EQ(record.word, Word{0x40A586B3});
    ASSERT_TRUE(record.regWrite.has_value());
    EXPECT_EQ(record.regWrite->rd, 13);
    EXPECT_EQ(record.regWrite->value, Word{2});
    EXPECT_FALSE(record.memWrite.has_value());
}

TEST(TestingCommitRecord, SwExample) {
    const CommitRecord record = sw_record();

    EXPECT_EQ(record.pc, Addr{0x80000010});
    EXPECT_EQ(record.word, Word{0xFEC12E23});
    EXPECT_FALSE(record.regWrite.has_value());
    ASSERT_TRUE(record.memWrite.has_value());
    EXPECT_EQ(record.memWrite->addr, Addr{0x800FFFEC});
    EXPECT_EQ(record.memWrite->width, std::size_t{4});
    EXPECT_EQ(record.memWrite->value, Word{12});
}

// A default-constructed record holds zeros and no writes: never garbage.
TEST(TestingCommitRecord, DefaultIsZeroWithNoWrites) {
    const CommitRecord record;

    EXPECT_EQ(record.pc, Addr{0});
    EXPECT_EQ(record.word, Word{0});
    EXPECT_FALSE(record.regWrite.has_value());
    EXPECT_FALSE(record.memWrite.has_value());
}

// ---- T3.4: records that differ in one detail are unequal ----

TEST(TestingCommitRecord, IdenticalRecordsAreEqual) {
    EXPECT_EQ(sub_record(), sub_record());
    EXPECT_EQ(sw_record(), sw_record());
    EXPECT_NE(sub_record(), sw_record());
}

// T3.4: same store, but one byte wide instead of four.
TEST(TestingCommitRecord, DifferOnlyInMemoryWriteWidth) {
    CommitRecord narrow = sw_record();
    narrow.memWrite->width = 1;

    EXPECT_NE(narrow, sw_record());
}

// T3.4: same instruction, but one has a register write and the other doesn't. An empty
// optional is unequal to any full one, even one holding x0 = 0.
TEST(TestingCommitRecord, DifferOnlyInWhetherTheyHaveARegisterWrite) {
    CommitRecord withoutWrite = sub_record();
    withoutWrite.regWrite = std::nullopt;
    EXPECT_NE(withoutWrite, sub_record());

    CommitRecord zeroWrite = sub_record();
    zeroWrite.regWrite = RegWrite{.rd = 0, .value = 0};
    EXPECT_NE(zeroWrite, withoutWrite);
}

// Beyond T3.4: every other field takes part in == too.
TEST(TestingCommitRecord, EveryFieldIsCompared) {
    CommitRecord record = sub_record();
    record.pc += 4;
    EXPECT_NE(record, sub_record());

    record = sub_record();
    record.word = 0x00B50633;   // add a2, a0, a1
    EXPECT_NE(record, sub_record());

    record = sub_record();
    record.regWrite->rd = 12;
    EXPECT_NE(record, sub_record());

    record = sub_record();
    record.regWrite->value = 3;
    EXPECT_NE(record, sub_record());

    record = sw_record();
    record.memWrite->addr = 0x800FFFF0;
    EXPECT_NE(record, sw_record());

    record = sw_record();
    record.memWrite->value = 13;
    EXPECT_NE(record, sw_record());

    record = sw_record();
    record.memWrite = std::nullopt;
    EXPECT_NE(record, sw_record());
}
