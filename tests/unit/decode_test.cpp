// Lab 5 tests, one block per class. Uncomment a class's line once you've written the class, and
// comment a line out to turn that class's tests off. Each block builds on its own and includes
// its own headers, so a block that's switched off doesn't need its class to exist yet.
//
// Every block uses the class and header names from the 5.3 and 5.4 tables in labs.md, and builds
// its class from (Addr pc, Word word), plus the AluOp for the two arithmetic classes. A class
// with a different name, header, or constructor needs the same change in its block.

// T5.3: the nine non-template classes (5.3)
#define RUN_T5_3_REGISTER_OP    // RegisterOpInstruction (add, sub, ...)
#define RUN_T5_3_IMMEDIATE_OP   // ImmediateOpInstruction (addi, slli, ...)
#define RUN_T5_3_LUI            // LuiInstruction
#define RUN_T5_3_AUIPC          // AuipcInstruction
// #define RUN_T5_3_JAL            // JalInstruction
// #define RUN_T5_3_JALR           // JalrInstruction
// #define RUN_T5_3_SYSTEM         // SystemInstruction (ecall, ebreak)
// #define RUN_T5_3_FENCE          // FenceInstruction
// #define RUN_T5_3_ILLEGAL        // IllegalInstruction

// T5.4: the three class templates (5.4), one switch per template for all its instantiations
// #define RUN_T5_4_LOAD           // LoadInstruction<T> (lb, lh, lw, lbu, lhu)
// #define RUN_T5_4_STORE          // StoreInstruction<T> (sb, sh, sw)
// #define RUN_T5_4_BRANCH         // BranchInstruction<Compare, T> (beq, bne, blt, ...)

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
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
