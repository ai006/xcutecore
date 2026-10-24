// Lab 5 tests: one block per class for T5.3 and T5.4, and one block for decode() (T5.5).
// Uncomment a line once you've written what it tests, and comment a line out to turn those tests
// off. Each block builds on its own and includes its own headers, so a block that's switched off
// doesn't need its files to exist yet.
//
// Every block uses the class and header names from the 5.3 and 5.4 tables in labs.md. The T5.3
// and T5.4 blocks build each class from (Addr pc, Word word), plus the AluOp for the two
// arithmetic classes; a class with a different name, header, or constructor needs the same
// change in its block. The T5.5 block calls decode(word, pc).

// T5.3: the nine non-template classes (5.3)
#define RUN_T5_3_REGISTER_OP    // RegisterOpInstruction (add, sub, ...)
#define RUN_T5_3_IMMEDIATE_OP   // ImmediateOpInstruction (addi, slli, ...)
#define RUN_T5_3_LUI            // LuiInstruction
#define RUN_T5_3_AUIPC          // AuipcInstruction
#define RUN_T5_3_JAL            // JalInstruction
#define RUN_T5_3_JALR           // JalrInstruction
#define RUN_T5_3_SYSTEM         // SystemInstruction (ecall, ebreak)
#define RUN_T5_3_FENCE          // FenceInstruction
#define RUN_T5_3_ILLEGAL        // IllegalInstruction

// T5.4: the three class templates (5.4), one switch per template for all its instantiations
#define RUN_T5_4_LOAD           // LoadInstruction<T> (lb, lh, lw, lbu, lhu)
#define RUN_T5_4_STORE          // StoreInstruction<T> (sb, sh, sw)
#define RUN_T5_4_BRANCH         // BranchInstruction<Compare, T> (beq, bne, blt, ...)

// T5.5: decode() (5.5), one switch for the whole function
#define RUN_T5_5                // decode(word, pc)

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "rvsim/instruction.hpp"
#include "rvsim/types.hpp"

#include "rv32i_vectors.hpp"

using rvsim::Addr;
using rvsim::RegIndex;
using rvsim::Word;

// Helpers shared by every block. They go in an unnamed namespace, as in fields_test.cpp, and
// each one is inline or a template, so it neither warns nor gets emitted while every block that
// uses it is switched off.
namespace {

using K = rvsim::InstructionKind;

// The mnemonic is objdump's text up to the first space: "addi" from "addi a0,zero,-1".
inline std::string
first_word(std::string_view text) {
    return std::string{text.substr(0, text.find(' '))};
}

// GoogleTest prints an enum class as raw bytes. Comparing names instead makes a failure read
// "ImmediateOp" against "Lui".
inline std::string_view
kind_name(rvsim::InstructionKind kind) {
    switch (kind) {
        case K::RegisterOp: return "RegisterOp";
        case K::ImmediateOp: return "ImmediateOp";
        case K::Lui: return "Lui";
        case K::Auipc: return "Auipc";
        case K::Load: return "Load";
        case K::Store: return "Store";
        case K::Branch: return "Branch";
        case K::Jal: return "Jal";
        case K::Jalr: return "Jalr";
        case K::System: return "System";
        case K::Fence: return "Fence";
        case K::Illegal: return "Illegal";
    }
    return "not an InstructionKind";
}

using MakeFn = std::unique_ptr<rvsim::Instruction> (*)(Addr, Word);

// Builds one class directly, the way decode() (5.5) will: from the pc and the word, plus the
// AluOp for the two arithmetic classes. It's a template, so each table row below names the exact
// class it means, template arguments included.
template <typename Class, auto... extra>
std::unique_ptr<rvsim::Instruction>
build(Addr pc, Word word) {
    return std::make_unique<Class>(pc, word, extra...);
}

// One mnemonic: which class it belongs to, and how to build that class.
struct ClassCase {
    std::string_view mnemonic;
    rvsim::InstructionKind kind;
    MakeFn make;
};

// One row of the second 5.3 table.
struct Queries {
    bool readsRs1;
    bool readsRs2;
    bool writesRd;
    bool isLoad;
    bool isControlFlow;
    bool isSystem;
};

// The second 5.3 table: every class's query answers. They depend on the class alone, never on
// the register numbers, so a row with rd = x0 (jal zero,18) still expects writes_rd() true.
inline Queries
expected_queries(rvsim::InstructionKind kind) {
    //                                rs1    rs2    rd     load   control system
    switch (kind) {
        case K::RegisterOp: return   {true,  true,  true,  false, false, false};
        case K::ImmediateOp: return  {true,  false, true,  false, false, false};
        case K::Lui: return          {false, false, true,  false, false, false};
        case K::Auipc: return        {false, false, true,  false, false, false};
        case K::Load: return         {true,  false, true,  true,  false, false};
        case K::Store: return        {true,  true,  false, false, false, false};
        case K::Branch: return       {true,  true,  false, false, true,  false};
        case K::Jal: return          {false, false, true,  false, true,  false};
        case K::Jalr: return         {true,  false, true,  false, true,  false};
        case K::System: return       {false, false, false, false, false, true};
        case K::Fence: return        {false, false, false, false, false, false};
        case K::Illegal: return      {false, false, false, false, false, false};
    }
    return {false, false, false, false, false, false};
}

// The table row for a mnemonic. Every Lab 4 row a test gets comes from rows_in on the same
// table, so the fallback never runs; it only keeps the function total.
template <std::size_t N>
const ClassCase&
case_for(const ClassCase (&cases)[N], std::string_view mnemonic) {
    for (const ClassCase& c : cases)
        if (c.mnemonic == mnemonic)
            return c;
    ADD_FAILURE() << "no table row for " << mnemonic;
    return cases[0];
}

// The Lab 4 rows whose mnemonic is in a table.
template <std::size_t N>
std::vector<TestVector>
rows_in(const ClassCase (&cases)[N]) {
    std::vector<TestVector> rows;
    for (const TestVector& v : rv32iVectors)
        for (const ClassCase& c : cases)
            if (c.mnemonic == first_word(v.text))
                rows.push_back(v);
    return rows;
}

// True if every mnemonic in a table matches at least one Lab 4 row. A misspelled mnemonic would
// match none, and rows_in would quietly leave its rows out of the test.
template <std::size_t N>
bool
every_mnemonic_has_a_row(const ClassCase (&cases)[N]) {
    for (const ClassCase& c : cases) {
        bool found = false;
        for (const TestVector& v : rv32iVectors)
            if (c.mnemonic == first_word(v.text))
                found = true;
        if (!found) {
            ADD_FAILURE() << "no Lab 4 row for " << c.mnemonic;
            return false;
        }
    }
    return true;
}

// Builds one class straight from a Lab 4 row and checks it against the row: the kind, the
// mnemonic objdump printed, the pc and word it was built from, the fields (the row holds 0 for
// any the format doesn't have), and the 5.3 query answers.
inline void
check_built(const TestVector& v, const ClassCase& c, Word expectedImm) {
    const std::unique_ptr<rvsim::Instruction> insn = c.make(v.pc, v.word);
    ASSERT_NE(insn.get(), nullptr);

    EXPECT_EQ(kind_name(insn->kind()), kind_name(c.kind));
    EXPECT_EQ(insn->mnemonic(), first_word(v.text));

    EXPECT_EQ(insn->pc(), v.pc);
    EXPECT_EQ(insn->word(), v.word);
    EXPECT_EQ(insn->rd(), v.rd);
    EXPECT_EQ(insn->rs1(), v.rs1);
    EXPECT_EQ(insn->rs2(), v.rs2);
    EXPECT_EQ(insn->imm(), expectedImm);

    const Queries q = expected_queries(c.kind);
    EXPECT_EQ(insn->reads_rs1(), q.readsRs1);
    EXPECT_EQ(insn->reads_rs2(), q.readsRs2);
    EXPECT_EQ(insn->writes_rd(), q.writesRd);
    EXPECT_EQ(insn->is_load(), q.isLoad);
    EXPECT_EQ(insn->is_control_flow(), q.isControlFlow);
    EXPECT_EQ(insn->is_system(), q.isSystem);
}

}

// ==== T5.3: RegisterOpInstruction (5.3) ====
#ifdef RUN_T5_3_REGISTER_OP

#include "rvsim/alu.hpp"
#include "rvsim/arith_instructions.hpp"

// Derived from Instruction and marked final, like every 5.3 class.
static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::RegisterOpInstruction>);
static_assert(std::is_final_v<rvsim::RegisterOpInstruction>);

namespace {

// The 10 R-type mnemonics, each with the AluOp the 5.2 table gives it.
const ClassCase registerOpCases[] = {
    {"add",  K::RegisterOp, &build<rvsim::RegisterOpInstruction, rvsim::AluOp::ADD>},
    {"sub",  K::RegisterOp, &build<rvsim::RegisterOpInstruction, rvsim::AluOp::SUB>},
    {"sll",  K::RegisterOp, &build<rvsim::RegisterOpInstruction, rvsim::AluOp::SLL>},
    {"slt",  K::RegisterOp, &build<rvsim::RegisterOpInstruction, rvsim::AluOp::SLT>},
    {"sltu", K::RegisterOp, &build<rvsim::RegisterOpInstruction, rvsim::AluOp::SLTU>},
    {"xor",  K::RegisterOp, &build<rvsim::RegisterOpInstruction, rvsim::AluOp::XOR>},
    {"srl",  K::RegisterOp, &build<rvsim::RegisterOpInstruction, rvsim::AluOp::SRL>},
    {"sra",  K::RegisterOp, &build<rvsim::RegisterOpInstruction, rvsim::AluOp::SRA>},
    {"or",   K::RegisterOp, &build<rvsim::RegisterOpInstruction, rvsim::AluOp::OR>},
    {"and",  K::RegisterOp, &build<rvsim::RegisterOpInstruction, rvsim::AluOp::AND>},
};

}

class TestingRegisterOpInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.3: built straight from each R-type Lab 4 word, with the AluOp for its mnemonic, it reports
// kind RegisterOp, objdump's mnemonic, rd, rs1, and rs2, an immediate of 0, and the 5.3 query
// answers.
TEST_P(TestingRegisterOpInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    check_built(v, case_for(registerOpCases, first_word(v.text)), v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingRegisterOpInstruction,
                         ::testing::ValuesIn(rows_in(registerOpCases)));

// T5.3: each AluOp gives its 5.2 mnemonic, and the AluOp alone decides it. The class never reads
// funct3 or funct7 again (that's decode()'s job), so the same add word, built with each AluOp,
// reports that AluOp's mnemonic.
TEST(TestingMnemonicChoice, RegisterOpFollowsTheAluOp) {
    const Word addWord = 0x00B50633;   // add a2,a0,a1
    for (const ClassCase& c : registerOpCases)
        EXPECT_EQ(c.make(0x80000008, addWord)->mnemonic(), c.mnemonic);
}

// Checks the test itself: every mnemonic in the table has a Lab 4 row (a typo would quietly skip
// its rows), and the table picks up all 10 R-type rows.
TEST(TestingCaseTables, RegisterOpRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(registerOpCases));
    EXPECT_EQ(rows_in(registerOpCases).size(), std::size_t{10});
}

#endif  // RUN_T5_3_REGISTER_OP

// ==== T5.3: ImmediateOpInstruction (5.3) ====
#ifdef RUN_T5_3_IMMEDIATE_OP

#include "rvsim/alu.hpp"
#include "rvsim/arith_instructions.hpp"

static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::ImmediateOpInstruction>);
static_assert(std::is_final_v<rvsim::ImmediateOpInstruction>);

namespace {

// The 9 I-type arithmetic mnemonics, each with the AluOp the 5.2 table gives it. There's no SUB:
// ADDI with a negative immediate does that job.
const ClassCase immediateOpCases[] = {
    {"addi",  K::ImmediateOp, &build<rvsim::ImmediateOpInstruction, rvsim::AluOp::ADD>},
    {"slti",  K::ImmediateOp, &build<rvsim::ImmediateOpInstruction, rvsim::AluOp::SLT>},
    {"sltiu", K::ImmediateOp, &build<rvsim::ImmediateOpInstruction, rvsim::AluOp::SLTU>},
    {"xori",  K::ImmediateOp, &build<rvsim::ImmediateOpInstruction, rvsim::AluOp::XOR>},
    {"ori",   K::ImmediateOp, &build<rvsim::ImmediateOpInstruction, rvsim::AluOp::OR>},
    {"andi",  K::ImmediateOp, &build<rvsim::ImmediateOpInstruction, rvsim::AluOp::AND>},
    {"slli",  K::ImmediateOp, &build<rvsim::ImmediateOpInstruction, rvsim::AluOp::SLL>},
    {"srli",  K::ImmediateOp, &build<rvsim::ImmediateOpInstruction, rvsim::AluOp::SRL>},
    {"srai",  K::ImmediateOp, &build<rvsim::ImmediateOpInstruction, rvsim::AluOp::SRA>},
};

// SLLI, SRLI, and SRAI keep only the shift amount, the low 5 bits of imm_i. The Lab 4 rows hold
// the whole imm_i (0x41F for srai a2,a3,0x1f, where bit 10 is what marks it as SRAI), so the test
// masks it.
bool
is_shift_immediate(std::string_view mnemonic) {
    return mnemonic == "slli" || mnemonic == "srli" || mnemonic == "srai";
}

}

class TestingImmediateOpInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.3: built straight from each OP-IMM Lab 4 word, with the AluOp for its mnemonic, it reports
// kind ImmediateOp, objdump's mnemonic, rd, rs1, the immediate, and the 5.3 query answers. rs2
// stays 0, even though those bits hold part of the immediate (31 in addi a0,zero,-1), and a
// shift's immediate is its shift amount, not the whole imm_i.
TEST_P(TestingImmediateOpInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    const ClassCase& c = case_for(immediateOpCases, first_word(v.text));
    check_built(v, c, is_shift_immediate(c.mnemonic) ? (v.imm & 0x1F) : v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingImmediateOpInstruction,
                         ::testing::ValuesIn(rows_in(immediateOpCases)));

// T5.3: each AluOp except SUB gives its 5.2 mnemonic, and the AluOp alone decides it, as in
// RegisterOpFollowsTheAluOp. The word is addi's, so slli, srli, and srai can't come from funct3.
TEST(TestingMnemonicChoice, ImmediateOpFollowsTheAluOp) {
    const Word addiWord = 0xFFF00513;   // addi a0,zero,-1
    for (const ClassCase& c : immediateOpCases)
        EXPECT_EQ(c.make(0x80000000, addiWord)->mnemonic(), c.mnemonic);
}

// Checks the test itself, as for RegisterOp: all 12 OP-IMM rows.
TEST(TestingCaseTables, ImmediateOpRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(immediateOpCases));
    EXPECT_EQ(rows_in(immediateOpCases).size(), std::size_t{12});
}

#endif  // RUN_T5_3_IMMEDIATE_OP

// ==== T5.3: LuiInstruction (5.3) ====
#ifdef RUN_T5_3_LUI

#include "rvsim/arith_instructions.hpp"

static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::LuiInstruction>);
static_assert(std::is_final_v<rvsim::LuiInstruction>);

namespace {

const ClassCase luiCases[] = {
    {"lui", K::Lui, &build<rvsim::LuiInstruction>},
};

}

class TestingLuiInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.3: built straight from each LUI Lab 4 word, it reports kind Lui, mnemonic lui, rd, the
// immediate from imm_u (the upper 20 bits in place), and the 5.3 query answers. rs1 stays 0,
// even though in lui a4,0x12345 those bits hold 8, part of the immediate.
TEST_P(TestingLuiInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    check_built(v, case_for(luiCases, first_word(v.text)), v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingLuiInstruction, ::testing::ValuesIn(rows_in(luiCases)));

// Checks the test itself: all 6 LUI rows.
TEST(TestingCaseTables, LuiRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(luiCases));
    EXPECT_EQ(rows_in(luiCases).size(), std::size_t{6});
}

#endif  // RUN_T5_3_LUI

// ==== T5.3: AuipcInstruction (5.3) ====
#ifdef RUN_T5_3_AUIPC

#include "rvsim/arith_instructions.hpp"

static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::AuipcInstruction>);
static_assert(std::is_final_v<rvsim::AuipcInstruction>);

namespace {

const ClassCase auipcCases[] = {
    {"auipc", K::Auipc, &build<rvsim::AuipcInstruction>},
};

}

class TestingAuipcInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.3: built straight from each AUIPC Lab 4 word, it reports kind Auipc, mnemonic auipc, rd, the
// immediate from imm_u, and the 5.3 query answers. It reads the pc, not a register, so it reads
// neither rs1 nor rs2.
TEST_P(TestingAuipcInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    check_built(v, case_for(auipcCases, first_word(v.text)), v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingAuipcInstruction,
                         ::testing::ValuesIn(rows_in(auipcCases)));

// Checks the test itself: all 3 AUIPC rows.
TEST(TestingCaseTables, AuipcRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(auipcCases));
    EXPECT_EQ(rows_in(auipcCases).size(), std::size_t{3});
}

#endif  // RUN_T5_3_AUIPC

// ==== T5.3: JalInstruction (5.3) ====
#ifdef RUN_T5_3_JAL

#include "rvsim/control_instructions.hpp"

static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::JalInstruction>);
static_assert(std::is_final_v<rvsim::JalInstruction>);

namespace {

const ClassCase jalCases[] = {
    {"jal", K::Jal, &build<rvsim::JalInstruction>},
};

}

class TestingJalInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.3: built straight from each JAL Lab 4 word, it reports kind Jal, mnemonic jal, rd, the
// offset from imm_j (the row's target minus its pc), and the 5.3 query answers. It writes rd even
// when rd is zero (jal zero,18): the answers depend on the class alone.
TEST_P(TestingJalInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    check_built(v, case_for(jalCases, first_word(v.text)), v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingJalInstruction, ::testing::ValuesIn(rows_in(jalCases)));

// Checks the test itself: all 8 JAL rows.
TEST(TestingCaseTables, JalRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(jalCases));
    EXPECT_EQ(rows_in(jalCases).size(), std::size_t{8});
}

#endif  // RUN_T5_3_JAL

// ==== T5.3: JalrInstruction (5.3) ====
#ifdef RUN_T5_3_JALR

#include "rvsim/control_instructions.hpp"

static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::JalrInstruction>);
static_assert(std::is_final_v<rvsim::JalrInstruction>);

namespace {

const ClassCase jalrCases[] = {
    {"jalr", K::Jalr, &build<rvsim::JalrInstruction>},
};

}

class TestingJalrInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.3: built straight from each JALR Lab 4 word, it reports kind Jalr, mnemonic jalr, rd, rs1,
// the immediate from imm_i, and the 5.3 query answers.
TEST_P(TestingJalrInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    check_built(v, case_for(jalrCases, first_word(v.text)), v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingJalrInstruction,
                         ::testing::ValuesIn(rows_in(jalrCases)));

// Checks the test itself: all 3 JALR rows.
TEST(TestingCaseTables, JalrRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(jalrCases));
    EXPECT_EQ(rows_in(jalrCases).size(), std::size_t{3});
}

#endif  // RUN_T5_3_JALR

// ==== T5.3: SystemInstruction (5.3) ====
#ifdef RUN_T5_3_SYSTEM

#include "rvsim/system_instructions.hpp"

static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::SystemInstruction>);
static_assert(std::is_final_v<rvsim::SystemInstruction>);

namespace {

// One class for both: the immediate tells them apart, 0 for ECALL and 1 for EBREAK.
const ClassCase systemCases[] = {
    {"ecall",  K::System, &build<rvsim::SystemInstruction>},
    {"ebreak", K::System, &build<rvsim::SystemInstruction>},
};

}

class TestingSystemInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.3: built straight from the ECALL and EBREAK words, it reports kind System, the right
// mnemonic, an immediate of 0 or 1, no registers, and the 5.3 query answers.
TEST_P(TestingSystemInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    check_built(v, case_for(systemCases, first_word(v.text)), v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingSystemInstruction,
                         ::testing::ValuesIn(rows_in(systemCases)));

// Checks the test itself: the ecall and ebreak rows.
TEST(TestingCaseTables, SystemRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(systemCases));
    EXPECT_EQ(rows_in(systemCases).size(), std::size_t{2});
}

#endif  // RUN_T5_3_SYSTEM

// ==== T5.3: FenceInstruction (5.3) ====
#ifdef RUN_T5_3_FENCE

#include "rvsim/system_instructions.hpp"

static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::FenceInstruction>);
static_assert(std::is_final_v<rvsim::FenceInstruction>);

namespace {

const ClassCase fenceCases[] = {
    {"fence", K::Fence, &build<rvsim::FenceInstruction>},
};

}

class TestingFenceInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.3: built straight from each FENCE Lab 4 word, it reports kind Fence, mnemonic fence, the
// immediate from imm_i (which holds the pred and succ sets: 0xFF for fence iorw,iorw), no
// registers, and every query false.
TEST_P(TestingFenceInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    check_built(v, case_for(fenceCases, first_word(v.text)), v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingFenceInstruction,
                         ::testing::ValuesIn(rows_in(fenceCases)));

// Checks the test itself: both FENCE rows.
TEST(TestingCaseTables, FenceRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(fenceCases));
    EXPECT_EQ(rows_in(fenceCases).size(), std::size_t{2});
}

#endif  // RUN_T5_3_FENCE

// ==== T5.3: IllegalInstruction (5.3) ====
#ifdef RUN_T5_3_ILLEGAL

#include "rvsim/system_instructions.hpp"

static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::IllegalInstruction>);
static_assert(std::is_final_v<rvsim::IllegalInstruction>);

// T5.3: an IllegalInstruction keeps the pc and the word and reports nothing else: kind Illegal,
// mnemonic illegal, every query false, and every field 0, even for the all-ones word, whose
// register bits would all give 31 if the class cut them out.
TEST(TestingIllegalInstruction, ReportsNothing) {
    for (const Word word : {Word{0x00000000}, Word{0xFFFFFFFF}, Word{0x02C58533}}) {
        SCOPED_TRACE(std::format("word 0x{:08x}", word));
        const rvsim::IllegalInstruction illegal(0x80000000, word);
        const rvsim::Instruction& insn = illegal;

        EXPECT_EQ(kind_name(insn.kind()), kind_name(K::Illegal));
        EXPECT_EQ(insn.mnemonic(), "illegal");
        EXPECT_EQ(insn.pc(), Addr{0x80000000});
        EXPECT_EQ(insn.word(), word);
        EXPECT_EQ(insn.rd(), RegIndex{0});
        EXPECT_EQ(insn.rs1(), RegIndex{0});
        EXPECT_EQ(insn.rs2(), RegIndex{0});
        EXPECT_EQ(insn.imm(), Word{0});
        EXPECT_FALSE(insn.reads_rs1());
        EXPECT_FALSE(insn.reads_rs2());
        EXPECT_FALSE(insn.writes_rd());
        EXPECT_FALSE(insn.is_load());
        EXPECT_FALSE(insn.is_control_flow());
        EXPECT_FALSE(insn.is_system());
    }
}

#endif  // RUN_T5_3_ILLEGAL

// ==== T5.4: LoadInstruction<T> (5.4) ====
#ifdef RUN_T5_4_LOAD

#include "rvsim/load_store_instructions.hpp"

// Each instantiation is its own class, derived from Instruction and marked final.
static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::LoadInstruction<std::int8_t>>);
static_assert(std::is_final_v<rvsim::LoadInstruction<std::int8_t>>);
static_assert(std::is_final_v<rvsim::LoadInstruction<std::int16_t>>);
static_assert(std::is_final_v<rvsim::LoadInstruction<std::int32_t>>);
static_assert(std::is_final_v<rvsim::LoadInstruction<std::uint8_t>>);
static_assert(std::is_final_v<rvsim::LoadInstruction<std::uint16_t>>);

namespace {

// The 5 loads, each with the T the 5.4 table gives it. LW is std::int32_t, so "unsigned means a u
// at the end" holds for every load.
const ClassCase loadCases[] = {
    {"lb",  K::Load, &build<rvsim::LoadInstruction<std::int8_t>>},
    {"lh",  K::Load, &build<rvsim::LoadInstruction<std::int16_t>>},
    {"lw",  K::Load, &build<rvsim::LoadInstruction<std::int32_t>>},
    {"lbu", K::Load, &build<rvsim::LoadInstruction<std::uint8_t>>},
    {"lhu", K::Load, &build<rvsim::LoadInstruction<std::uint16_t>>},
};

}

class TestingLoadInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.4: built straight from each load's Lab 4 word, its instantiation reports kind Load, objdump's
// mnemonic, rd, rs1, the immediate from imm_i, and the 5.3 query answers.
TEST_P(TestingLoadInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    check_built(v, case_for(loadCases, first_word(v.text)), v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingLoadInstruction, ::testing::ValuesIn(rows_in(loadCases)));

// T5.4: T alone decides the mnemonic, so every instantiation built from the same lb word still
// reports its own. lb and lbu differ only in T.
TEST(TestingMnemonicChoice, LoadFollowsT) {
    const Word lbWord = 0x00058503;   // lb a0,0(a1)
    for (const ClassCase& c : loadCases)
        EXPECT_EQ(c.make(0x80000000, lbWord)->mnemonic(), c.mnemonic);
}

// Checks the test itself: all 5 load rows.
TEST(TestingCaseTables, LoadRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(loadCases));
    EXPECT_EQ(rows_in(loadCases).size(), std::size_t{5});
}

#endif  // RUN_T5_4_LOAD

// ==== T5.4: StoreInstruction<T> (5.4) ====
#ifdef RUN_T5_4_STORE

#include "rvsim/load_store_instructions.hpp"

static_assert(std::is_base_of_v<rvsim::Instruction, rvsim::StoreInstruction<std::uint8_t>>);
static_assert(std::is_final_v<rvsim::StoreInstruction<std::uint8_t>>);
static_assert(std::is_final_v<rvsim::StoreInstruction<std::uint16_t>>);
static_assert(std::is_final_v<rvsim::StoreInstruction<std::uint32_t>>);

namespace {

// The 3 stores, each with the T the 5.4 table gives it.
const ClassCase storeCases[] = {
    {"sb", K::Store, &build<rvsim::StoreInstruction<std::uint8_t>>},
    {"sh", K::Store, &build<rvsim::StoreInstruction<std::uint16_t>>},
    {"sw", K::Store, &build<rvsim::StoreInstruction<std::uint32_t>>},
};

}

class TestingStoreInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.4: built straight from each store's Lab 4 word, its instantiation reports kind Store,
// objdump's mnemonic, rs1, rs2, the immediate from imm_s, and the 5.3 query answers. rd stays 0,
// even though in sw a2,-4(sp) those bits hold 28, the low bits of the -4.
TEST_P(TestingStoreInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    check_built(v, case_for(storeCases, first_word(v.text)), v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingStoreInstruction,
                         ::testing::ValuesIn(rows_in(storeCases)));

// T5.4: T alone decides the mnemonic, so every instantiation built from the same sw word still
// reports its own.
TEST(TestingMnemonicChoice, StoreFollowsT) {
    const Word swWord = 0xFEC12E23;   // sw a2,-4(sp)
    for (const ClassCase& c : storeCases)
        EXPECT_EQ(c.make(0x80000000, swWord)->mnemonic(), c.mnemonic);
}

// Checks the test itself: all 7 store rows.
TEST(TestingCaseTables, StoreRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(storeCases));
    EXPECT_EQ(rows_in(storeCases).size(), std::size_t{7});
}

#endif  // RUN_T5_4_STORE

// ==== T5.4: BranchInstruction<Compare, T> (5.4) ====
#ifdef RUN_T5_4_BRANCH

#include "rvsim/control_instructions.hpp"

static_assert(std::is_base_of_v<rvsim::Instruction,
                                rvsim::BranchInstruction<std::equal_to<>, std::uint32_t>>);
static_assert(std::is_final_v<rvsim::BranchInstruction<std::equal_to<>, std::uint32_t>>);
static_assert(std::is_final_v<rvsim::BranchInstruction<std::not_equal_to<>, std::uint32_t>>);
static_assert(std::is_final_v<rvsim::BranchInstruction<std::less<>, std::int32_t>>);
static_assert(std::is_final_v<rvsim::BranchInstruction<std::greater_equal<>, std::int32_t>>);
static_assert(std::is_final_v<rvsim::BranchInstruction<std::less<>, std::uint32_t>>);
static_assert(std::is_final_v<rvsim::BranchInstruction<std::greater_equal<>, std::uint32_t>>);

namespace {

// The 6 branches, each with the Compare and T the 5.4 table gives it.
const ClassCase branchCases[] = {
    {"beq",  K::Branch, &build<rvsim::BranchInstruction<std::equal_to<>, std::uint32_t>>},
    {"bne",  K::Branch, &build<rvsim::BranchInstruction<std::not_equal_to<>, std::uint32_t>>},
    {"blt",  K::Branch, &build<rvsim::BranchInstruction<std::less<>, std::int32_t>>},
    {"bge",  K::Branch, &build<rvsim::BranchInstruction<std::greater_equal<>, std::int32_t>>},
    {"bltu", K::Branch, &build<rvsim::BranchInstruction<std::less<>, std::uint32_t>>},
    {"bgeu", K::Branch, &build<rvsim::BranchInstruction<std::greater_equal<>, std::uint32_t>>},
};

}

class TestingBranchInstruction : public ::testing::TestWithParam<TestVector> {};

// T5.4: built straight from each branch's Lab 4 word, its instantiation reports kind Branch,
// objdump's mnemonic, rs1, rs2, the offset from imm_b (the row's target minus its pc), and the
// 5.3 query answers.
TEST_P(TestingBranchInstruction, ReportsKindMnemonicFieldsAndQueries) {
    const TestVector& v = GetParam();
    check_built(v, case_for(branchCases, first_word(v.text)), v.imm);
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingBranchInstruction,
                         ::testing::ValuesIn(rows_in(branchCases)));

// T5.4: Compare and T alone decide the mnemonic, so every instantiation built from the same beq
// word still reports its own. blt and bltu differ only in T.
TEST(TestingMnemonicChoice, BranchFollowsCompareAndT) {
    const Word beqWord = 0x00B50463;   // beq a0,a1,+8
    for (const ClassCase& c : branchCases)
        EXPECT_EQ(c.make(0x80000000, beqWord)->mnemonic(), c.mnemonic);
}

// Checks the test itself: all 8 branch rows.
TEST(TestingCaseTables, BranchRows) {
    EXPECT_TRUE(every_mnemonic_has_a_row(branchCases));
    EXPECT_EQ(rows_in(branchCases).size(), std::size_t{8});
}

#endif  // RUN_T5_4_BRANCH

// ==== T5.5: decode() (5.5) ====
#ifdef RUN_T5_5

#include "rvsim/control_instructions.hpp"
#include "rvsim/decode.hpp"
#include "rvsim/load_store_instructions.hpp"

// decode() takes the word and the pc and returns the object behind a std::unique_ptr.
static_assert(std::is_same_v<decltype(rvsim::decode(Word{}, Addr{})),
                             std::unique_ptr<rvsim::Instruction>>);

namespace {

// objdump's mnemonic and the class decode() must choose for it: all 40 RV32I instructions.
struct MnemonicKind {
    std::string_view mnemonic;
    rvsim::InstructionKind kind;
};

const MnemonicKind allMnemonics[] = {
    {"add", K::RegisterOp},   {"sub", K::RegisterOp},   {"sll", K::RegisterOp},
    {"slt", K::RegisterOp},   {"sltu", K::RegisterOp},  {"xor", K::RegisterOp},
    {"srl", K::RegisterOp},   {"sra", K::RegisterOp},   {"or", K::RegisterOp},
    {"and", K::RegisterOp},
    {"addi", K::ImmediateOp}, {"slti", K::ImmediateOp}, {"sltiu", K::ImmediateOp},
    {"xori", K::ImmediateOp}, {"ori", K::ImmediateOp},  {"andi", K::ImmediateOp},
    {"slli", K::ImmediateOp}, {"srli", K::ImmediateOp}, {"srai", K::ImmediateOp},
    {"lui", K::Lui},          {"auipc", K::Auipc},
    {"lb", K::Load},          {"lh", K::Load},          {"lw", K::Load},
    {"lbu", K::Load},         {"lhu", K::Load},
    {"sb", K::Store},         {"sh", K::Store},         {"sw", K::Store},
    {"beq", K::Branch},       {"bne", K::Branch},       {"blt", K::Branch},
    {"bge", K::Branch},       {"bltu", K::Branch},      {"bgeu", K::Branch},
    {"jal", K::Jal},          {"jalr", K::Jalr},
    {"ecall", K::System},     {"ebreak", K::System},    {"fence", K::Fence},
};

static_assert(std::size(allMnemonics) == 40);

rvsim::InstructionKind
expected_kind(std::string_view mnemonic) {
    for (const MnemonicKind& m : allMnemonics)
        if (m.mnemonic == mnemonic)
            return m.kind;
    ADD_FAILURE() << "not an RV32I mnemonic: " << mnemonic;
    return K::Illegal;
}

// The 5.5 examples and the decode column of Lab 5's worked examples.
struct DecodeCase {
    Word word;
    Addr pc;
    rvsim::InstructionKind kind;
    std::string_view mnemonic;
};

// How GoogleTest names a row in CTest: the pc and the word in hex.
inline void
PrintTo(const DecodeCase& c, std::ostream* os) {
    *os << std::format("{:08x}_{:08x}", c.pc, c.word);
}

const DecodeCase decodeCases[] = {
    {0x00B50633, 0x80000008, K::RegisterOp,  "add"},      // add a2,a0,a1
    {0x40A586B3, 0x8000000C, K::RegisterOp,  "sub"},      // sub a3,a1,a0
    {0xFFF00513, 0x80000000, K::ImmediateOp, "addi"},     // addi a0,zero,-1
    {0x41F6D613, 0x80000000, K::ImmediateOp, "srai"},     // srai a2,a3,0x1f
    {0x80073693, 0x80000000, K::ImmediateOp, "sltiu"},    // sltiu a3,a4,-2048
    {0x12345737, 0x80000000, K::Lui,         "lui"},      // lui a4,0x12345
    {0x00001517, 0x80000000, K::Auipc,       "auipc"},    // auipc a0,0x1
    {0x00812783, 0x80000000, K::Load,        "lw"},       // lw a5,8(sp)
    {0x7FF3C303, 0x80000000, K::Load,        "lbu"},      // lbu t1,2047(t2)
    {0xFEC12E23, 0x80000010, K::Store,       "sw"},       // sw a2,-4(sp)
    {0x00B50463, 0x80000014, K::Branch,      "beq"},      // beq a0,a1,8000001c
    {0x010000EF, 0x80000014, K::Jal,         "jal"},      // jal ra,80000024
    {0x00008067, 0x80000000, K::Jalr,        "jalr"},     // jalr zero,0(ra)
    {0x00000073, 0x80000000, K::System,      "ecall"},    // ecall
    {0x00100073, 0x80000000, K::System,      "ebreak"},   // ebreak
    {0x0FF0000F, 0x80000000, K::Fence,       "fence"},    // fence iorw,iorw
    {0x00000000, 0x80000000, K::Illegal,     "illegal"},  // all zeros
    {0x02C58533, 0x80000000, K::Illegal,     "illegal"},  // mul a0,a1,a2, from RV32M
};

// Words decode() must reject. Each one comes from tests/programs/lab5/illegal.S, built by the
// cross assembler (illegal.dump lists them); the comment after a row is the line that built it.
struct IllegalWord {
    Word word;
    std::string_view why;
};

// How GoogleTest names a row in CTest: the word in hex.
inline void
PrintTo(const IllegalWord& w, std::ostream* os) {
    *os << std::format("0x{:08x}", w.word);
}

const IllegalWord illegalWords[] = {
    {0x00000000, "all zeros"},                                  // .word 0x00000000
    {0xFFFFFFFF, "all ones"},                                   // .word 0xffffffff
    {0x00C5850B, "custom-0"},                                   // .insn r 0x0b, 0, 0, a0, a1, a2
    {0x00C5857B, "custom-3"},                                   // .insn r 0x7b, 0, 0, a0, a1, a2
    {0x00004501, "c.li a0,0 (RVC): low two bits 01, not 11"},   // .word 0x00004501
    {0x0005A507, "LOAD-FP, from F"},                            // flw fa0, 0(a1)
    {0x00C5F553, "OP-FP, from F"},                              // fadd.s fa0, fa1, fa2
    {0x0015851B, "OP-IMM-32, RV64 only"},                       // addiw a0, a1, 1
    {0x00C5853B, "OP-32, RV64 only"},                           // addw a0, a1, a2
    {0x000510E7, "JALR with funct3 001"},                       // .insn i 0x67, 1, ra, a0, 0
    {0x000570E7, "JALR with funct3 111"},                       // .insn i 0x67, 7, ra, a0, 0
    {0xFCB52AE3, "BRANCH funct3 010"},                          // .insn b 0x63, 2, a0, a1, _start
    {0xFCB538E3, "BRANCH funct3 011"},                          // .insn b 0x63, 3, a0, a1, _start
    {0x0005B503, "LOAD funct3 011: RV64 LD"},                   // ld a0, 0(a1)
    {0x0005E503, "LOAD funct3 110: RV64 LWU"},                  // lwu a0, 0(a1)
    {0x0005F503, "LOAD funct3 111"},                            // .insn i 0x03, 7, a0, a1, 0
    {0x00A5B023, "STORE funct3 011: RV64 SD"},                  // sd a0, 0(a1)
    {0x00A5C023, "STORE funct3 100"},                           // .insn s 0x23, 4, a0, 0(a1)
    {0x00A5F023, "STORE funct3 111"},                           // .insn s 0x23, 7, a0, 0(a1)
    {0x02C58533, "funct7 0000001: RV32M MUL"},                  // mul a0, a1, a2
    {0x02C5D533, "funct7 0000001: RV32M DIVU"},                 // divu a0, a1, a2
    {0x40C59533, "funct7 0100000 with SLL's funct3"},           // .insn r 0x33, 1, 0x20, a0, a1, a2
    {0x80C58533, "funct7 1000000"},                             // .insn r 0x33, 0, 0x40, a0, a1, a2
    {0x02059513, "funct7 0000001: RV64 shift by 32"},           // slli a0, a1, 32
    {0x4205D513, "funct7 0100001: RV64 shift by 32"},           // srai a0, a1, 32
    {0x40059513, "SLLI with funct7 0100000"},                   // .insn i 0x13, 1, a0, a1, 0x400
    {0xFFF5D513, "SRLI or SRAI with funct7 1111111"},           // .insn i 0x13, 5, a0, a1, -1
    {0x0000100F, "FENCE.I"},                                    // fence.i
    {0x30059573, "Zicsr CSRRW"},                                // csrrw a0, mstatus, a1
    {0xC0002573, "Zicsr CSRRS (rdcycle)"},                      // csrrs a0, cycle, zero
    {0x30200073, "privileged MRET"},                            // mret
    {0x10500073, "privileged WFI"},                             // wfi
    {0x000000F3, "ECALL with rd = x1"},                         // .insn i 0x73, 0, ra, zero, 0
    {0x00050073, "ECALL with rs1 = x10"},                       // .insn i 0x73, 0, zero, a0, 0
    {0x001000F3, "EBREAK with rd = x1"},                        // .insn i 0x73, 0, ra, zero, 1
};

static_assert(std::size(illegalWords) == 35);

// Beyond T5.5: 5.5's table, written again from the spec with raw bit arithmetic, so it shares no
// code with your decode().
rvsim::InstructionKind
kind_from_the_5_5_table(Word word) {
    const Word op = word & 0x7F;
    const Word f3 = (word >> 12) & 0x7;
    const Word f7 = word >> 25;
    switch (op) {
        case 0x37: return K::Lui;                                           // LUI
        case 0x17: return K::Auipc;                                         // AUIPC
        case 0x6F: return K::Jal;                                           // JAL
        case 0x67: return f3 == 0 ? K::Jalr : K::Illegal;                   // JALR
        case 0x63: return (f3 == 2 || f3 == 3) ? K::Illegal : K::Branch;    // BRANCH
        case 0x03: return (f3 == 3 || f3 >= 6) ? K::Illegal : K::Load;      // LOAD
        case 0x23: return f3 <= 2 ? K::Store : K::Illegal;                  // STORE
        case 0x13:                                                          // OP-IMM
            if (f3 == 1)
                return f7 == 0x00 ? K::ImmediateOp : K::Illegal;
            if (f3 == 5)
                return (f7 == 0x00 || f7 == 0x20) ? K::ImmediateOp : K::Illegal;
            return K::ImmediateOp;
        case 0x33:                                                          // OP
            if (f7 == 0x00)
                return K::RegisterOp;
            return (f7 == 0x20 && (f3 == 0 || f3 == 5)) ? K::RegisterOp : K::Illegal;
        case 0x0F: return f3 == 0 ? K::Fence : K::Illegal;                  // MISC-MEM
        case 0x73:                                                          // SYSTEM
            return (word == 0x00000073 || word == 0x00100073) ? K::System : K::Illegal;
        default: return K::Illegal;
    }
}

// Beyond T5.5: true if the object is exactly this class, template arguments included.
template <typename Class>
bool
is_a(const rvsim::Instruction& insn) {
    return dynamic_cast<const Class*>(&insn) != nullptr;
}

// One template mnemonic and the instantiation the 5.4 table gives it.
struct TemplateCase {
    std::string_view mnemonic;
    std::string_view instantiation;
    bool (*isIt)(const rvsim::Instruction&);
};

// How GoogleTest names a row in CTest: the mnemonic.
inline void
PrintTo(const TemplateCase& c, std::ostream* os) {
    *os << c.mnemonic;
}

const TemplateCase templateCases[] = {
    {"lb",   "LoadInstruction<std::int8_t>",   &is_a<rvsim::LoadInstruction<std::int8_t>>},
    {"lh",   "LoadInstruction<std::int16_t>",  &is_a<rvsim::LoadInstruction<std::int16_t>>},
    {"lw",   "LoadInstruction<std::int32_t>",  &is_a<rvsim::LoadInstruction<std::int32_t>>},
    {"lbu",  "LoadInstruction<std::uint8_t>",  &is_a<rvsim::LoadInstruction<std::uint8_t>>},
    {"lhu",  "LoadInstruction<std::uint16_t>", &is_a<rvsim::LoadInstruction<std::uint16_t>>},
    {"sb",   "StoreInstruction<std::uint8_t>",  &is_a<rvsim::StoreInstruction<std::uint8_t>>},
    {"sh",   "StoreInstruction<std::uint16_t>", &is_a<rvsim::StoreInstruction<std::uint16_t>>},
    {"sw",   "StoreInstruction<std::uint32_t>", &is_a<rvsim::StoreInstruction<std::uint32_t>>},
    {"beq",  "BranchInstruction<std::equal_to<>, std::uint32_t>",
             &is_a<rvsim::BranchInstruction<std::equal_to<>, std::uint32_t>>},
    {"bne",  "BranchInstruction<std::not_equal_to<>, std::uint32_t>",
             &is_a<rvsim::BranchInstruction<std::not_equal_to<>, std::uint32_t>>},
    {"blt",  "BranchInstruction<std::less<>, std::int32_t>",
             &is_a<rvsim::BranchInstruction<std::less<>, std::int32_t>>},
    {"bge",  "BranchInstruction<std::greater_equal<>, std::int32_t>",
             &is_a<rvsim::BranchInstruction<std::greater_equal<>, std::int32_t>>},
    {"bltu", "BranchInstruction<std::less<>, std::uint32_t>",
             &is_a<rvsim::BranchInstruction<std::less<>, std::uint32_t>>},
    {"bgeu", "BranchInstruction<std::greater_equal<>, std::uint32_t>",
             &is_a<rvsim::BranchInstruction<std::greater_equal<>, std::uint32_t>>},
};

static_assert(std::size(templateCases) == 14);

// The first ten lines of a list of mismatches, so one bug doesn't print thousands of failures.
std::string
first_ten(const std::vector<std::string>& lines) {
    std::string text;
    for (std::size_t i = 0; i < lines.size() && i < 10; i++)
        text += "\n  " + lines[i];
    return text;
}

}

class TestingDecode : public ::testing::TestWithParam<TestVector> {};

// T5.5: every Lab 4 vector decodes to the class its mnemonic belongs to. The mnemonic also shows
// decode() chose the right AluOp or template arguments: addi and slti differ only there.
TEST_P(TestingDecode, VectorDecodesToTheRightClass) {
    const TestVector& v = GetParam();
    const std::unique_ptr<rvsim::Instruction> insn = rvsim::decode(v.word, v.pc);
    ASSERT_NE(insn.get(), nullptr);

    EXPECT_EQ(kind_name(insn->kind()), kind_name(expected_kind(first_word(v.text))));
    EXPECT_EQ(insn->mnemonic(), first_word(v.text));
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingDecode, ::testing::ValuesIn(rv32iVectors));

class TestingDecodeExamples : public ::testing::TestWithParam<DecodeCase> {};

// T5.5: the 5.5 examples and the worked examples, each at its own pc.
TEST_P(TestingDecodeExamples, DecodesToTheRightClass) {
    const DecodeCase& c = GetParam();
    const std::unique_ptr<rvsim::Instruction> insn = rvsim::decode(c.word, c.pc);
    ASSERT_NE(insn.get(), nullptr);

    EXPECT_EQ(kind_name(insn->kind()), kind_name(c.kind));
    EXPECT_EQ(insn->mnemonic(), c.mnemonic);
}

INSTANTIATE_TEST_SUITE_P(Examples, TestingDecodeExamples, ::testing::ValuesIn(decodeCases));

class TestingDecodeIllegal : public ::testing::TestWithParam<IllegalWord> {};

// T5.5: every word from illegal.S becomes an IllegalInstruction, not an error.
TEST_P(TestingDecodeIllegal, BecomesIllegalInstruction) {
    const IllegalWord& w = GetParam();
    SCOPED_TRACE(w.why);
    const std::unique_ptr<rvsim::Instruction> insn = rvsim::decode(w.word, 0x80000000);
    ASSERT_NE(insn.get(), nullptr);

    EXPECT_EQ(kind_name(insn->kind()), kind_name(K::Illegal));
    EXPECT_EQ(insn->mnemonic(), "illegal");
}

INSTANTIATE_TEST_SUITE_P(IllegalS, TestingDecodeIllegal, ::testing::ValuesIn(illegalWords));

class TestingDecodeTemplateArguments : public ::testing::TestWithParam<TemplateCase> {};

// Beyond T5.5: each load, store, and branch decodes to the exact instantiation the 5.4 table
// gives, checked with dynamic_cast. The mnemonic alone doesn't prove it, and Lab 6 computes with
// Compare and T: the branch decision (6.4) and the load's extension (6.5).
TEST_P(TestingDecodeTemplateArguments, BuildsThe5_4Instantiation) {
    const TemplateCase& c = GetParam();
    std::size_t rows = 0;
    for (const TestVector& v : rv32iVectors) {
        if (first_word(v.text) != c.mnemonic)
            continue;
        rows++;
        const std::unique_ptr<rvsim::Instruction> insn = rvsim::decode(v.word, v.pc);
        ASSERT_NE(insn.get(), nullptr) << v.text;
        EXPECT_TRUE(c.isIt(*insn)) << v.text << " should decode to a " << c.instantiation;
    }
    // Checks the test itself: a misspelled mnemonic would match no Lab 4 row.
    EXPECT_GT(rows, std::size_t{0}) << "no Lab 4 row for " << c.mnemonic;
}

INSTANTIATE_TEST_SUITE_P(Templates, TestingDecodeTemplateArguments,
                         ::testing::ValuesIn(templateCases));

// Beyond T5.5: decode() builds the object for the word and the pc it was given, so word() and
// pc() read them back unchanged: every vector, every example, and every illegal word.
TEST(TestingDecodePassThrough, KeepsTheWordAndThePc) {
    std::vector<std::string> wrong;
    const auto check = [&wrong](Word word, Addr pc) {
        const std::unique_ptr<rvsim::Instruction> insn = rvsim::decode(word, pc);
        if (insn == nullptr)
            wrong.push_back(std::format("decode(0x{:08x}, 0x{:08x}): null", word, pc));
        else if (insn->word() != word || insn->pc() != pc)
            wrong.push_back(std::format("decode(0x{:08x}, 0x{:08x}): word() 0x{:08x}, pc() 0x{:08x}",
                                        word, pc, insn->word(), insn->pc()));
    };
    for (const TestVector& v : rv32iVectors)
        check(v.word, v.pc);
    for (const DecodeCase& c : decodeCases)
        check(c.word, c.pc);
    for (const IllegalWord& w : illegalWords)
        check(w.word, 0x80000000);

    EXPECT_TRUE(wrong.empty()) << wrong.size() << " objects don't keep their word and pc, first ones:"
                               << first_ten(wrong);
}

// Beyond T5.5: the spec makes every MISC-MEM word with funct3 000 a FENCE, whatever rd, rs1, and
// fm hold. This objdump prints the first two as raw words; decode() still takes them.
TEST(TestingDecodeEdges, EveryFunct3ZeroMiscMemWordIsAFence) {
    for (const Word word : {Word{0x8330000F},     // fence.tso: fm 1000
                            Word{0x0FF5050F},     // fence iorw,iorw with rd = rs1 = a0
                            Word{0x0100000F}}) {  // pred w, succ empty
        SCOPED_TRACE(std::format("word 0x{:08x}", word));
        const std::unique_ptr<rvsim::Instruction> insn = rvsim::decode(word, 0x80000000);
        ASSERT_NE(insn.get(), nullptr);
        EXPECT_EQ(kind_name(insn->kind()), kind_name(K::Fence));
        EXPECT_EQ(insn->mnemonic(), "fence");
    }
}

// Beyond T5.5: decode() never returns null and never throws, and its class matches 5.5's table,
// for every opcode with every funct3, four funct7 values, and two sets of registers (8192 words).
// Mismatches are collected so one bug doesn't print thousands of failures.
TEST(TestingDecodeSweep, EveryOpcodeFunct3AndFunct7MatchesTheTable) {
    std::vector<std::string> wrong;
    for (Word op = 0; op < 128; op++)
        for (Word f3 = 0; f3 < 8; f3++)
            for (const Word f7 : {Word{0x00}, Word{0x01}, Word{0x20}, Word{0x7F}})
                for (const Word regs : {Word{0}, Word{1}}) {
                    // rd, rs1, rs2 are all 0, or x5, x6, x7.
                    const Word rd = regs * 5, rs1 = regs * 6, rs2 = regs * 7;
                    const Word word = (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12)
                                      | (rd << 7) | op;
                    const rvsim::InstructionKind expected = kind_from_the_5_5_table(word);
                    try {
                        const std::unique_ptr<rvsim::Instruction> insn =
                            rvsim::decode(word, 0x80000000);
                        if (insn == nullptr)
                            wrong.push_back(std::format("0x{:08x}: null", word));
                        else if (insn->kind() != expected)
                            wrong.push_back(std::format("0x{:08x}: {}, the table says {}", word,
                                                        kind_name(insn->kind()),
                                                        kind_name(expected)));
                    } catch (...) {
                        wrong.push_back(std::format("0x{:08x}: threw", word));
                    }
                }

    EXPECT_TRUE(wrong.empty()) << wrong.size() << " words decode differently, first ones:"
                               << first_ten(wrong);
}

#endif  // RUN_T5_5
