# RV32I CPU Model in C++: Lab Path

You are building a small computer in C++: a RISC-V RV32I CPU and a memory, connected by a bus, with instructions and data sharing one address space (Von Neumann). The CPU starts unpipelined: each instruction goes through fetch, decode, execute, memory, and write back before the next one starts. Once it runs real programs and passes the official tests, you turn it into a cycle-level 5-stage pipeline, then add branch prediction and caches.

Every lab lists objectives, tests, a "done when" check, and the C++ it exercises. Lab 0 is the exception: a step-by-step setup walkthrough with code. Objectives are numbered by lab (2.3 is Lab 2, objective 3), and `hints.md` uses the same numbers. Try each objective before opening its hint.

## Ground rules

- Write the tests for an objective before starting the next objective.
- Never leave an earlier lab's tests failing.
- Run tests in the sanitizer build by default.
- Commit after each objective. Tag a release at the end of each phase.
- Record each design decision in `DESIGN.md`: the choice and the reason, a few lines each.

## Roadmap

| Phase | Labs | Milestone | Tag |
|---|---|---|---|
| 0. Setup | 0 | Build, unit tests, and cross toolchain work | |
| 1. Architectural state | 1, 2, 3 | Bit helpers, memory and bus, registers | |
| 2. Decode | 4, 5 | Every RV32I instruction decodes and disassembles | |
| 3. Execute | 6 | Every instruction's behavior is unit tested | |
| 4. Unpipelined CPU | 7, 8, 9 | Runs your C programs and the official rv32ui tests | v0.1 |
| 5. Pipeline | 10, 11, 12 | Pipeline commits exactly what the unpipelined CPU commits | v0.2 |
| 6. Branch prediction | 13, 14 | BTB and bimodal predictor, with measured results | v0.3 |
| 7. Caches | 15, 16 | Cache hierarchy with timing, with measured results | v0.4 |
| 8. Extensions | menu | Pick what interests you | |

Lab sizes (S, M, L) are relative effort.

---

## Phase 0: Setup

### Lab 0: Project setup (walkthrough)

Lab 0 is a set of steps to follow, with code. At the end you have a build, a working test harness, and a checked cross toolchain. The names `rvsim` (project, namespace, executable) and `rvcore` (library) are placeholders; rename them if you like.

#### Step 0.1: Install the tools

```bash
sudo apt update
sudo apt install build-essential cmake git clang-format libgtest-dev \
                 gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
g++ --version
cmake --version
```

You need g++ 11 or newer and CMake 3.22 or newer. Ubuntu 22.04's g++ 11 has most of C++20 but not `std::format`. If you want `std::format`, install GCC 13 and add `-DCMAKE_CXX_COMPILER=g++-13` to the configure commands in Step 0.5:

```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt install g++-13
```

#### Step 0.2: Lay out the repository

Target layout:

```
.
├── CMakeLists.txt
├── README.md
├── .clang-format
├── .gitignore
├── docs/
│   ├── DESIGN.md
│   ├── hints.md
│   └── labs.md
├── include/
│   └── rvsim/
├── sim/
├── src/
└── tests/
    ├── programs/
    │   └── lab0/
    └── unit/
```

From your current tree:

```bash
mv apps sim                  # skip if you already renamed it
mv labs.md hints.md docs/
mkdir -p include/rvsim tests/programs/lab0
touch README.md
```

#### Step 0.3: Add placeholder code

A library needs at least one source file, and the test needs something to call.

`include/rvsim/version.hpp`

```cpp
#pragma once

#include <string_view>

namespace rvsim {

[[nodiscard]] std::string_view version() noexcept;

}  // namespace rvsim
```

`src/version.cpp`

```cpp
#include "rvsim/version.hpp"

namespace rvsim {

std::string_view version() noexcept { return "0.0.1"; }

}  // namespace rvsim
```

`sim/main.cpp`

```cpp
#include <iostream>

#include "rvsim/version.hpp"

int main() {
  std::cout << "rvsim " << rvsim::version() << '\n';
  return 0;
}
```

`tests/unit/smoke_test.cpp`

```cpp
#include <gtest/gtest.h>

#include "rvsim/version.hpp"

TEST(Smoke, VersionIsNotEmpty) {
  EXPECT_FALSE(rvsim::version().empty());
}
```

#### Step 0.4: Write the CMake files

`CMakeLists.txt` (top level)

```cmake
cmake_minimum_required(VERSION 3.22)
project(rvsim LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RVSIM_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Warnings and sanitizers, applied only to targets that link this.
add_library(rvsim_options INTERFACE)
target_compile_options(rvsim_options INTERFACE
  -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
  -Wshadow -Wswitch-enum -Werror)
if(RVSIM_SANITIZE)
  target_compile_options(rvsim_options INTERFACE
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
  target_link_options(rvsim_options INTERFACE -fsanitize=address,undefined)
endif()

add_subdirectory(src)
add_subdirectory(sim)

enable_testing()
add_subdirectory(tests)
```

`src/CMakeLists.txt`

```cmake
add_library(rvcore STATIC
  version.cpp
)
target_include_directories(rvcore PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(rvcore PUBLIC cxx_std_20)
target_link_libraries(rvcore PRIVATE rvsim_options)
```

`sim/CMakeLists.txt`

```cmake
add_executable(rvsim main.cpp)
target_link_libraries(rvsim PRIVATE rvcore rvsim_options)
```

`tests/CMakeLists.txt`

```cmake
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(unit_tests
  unit/smoke_test.cpp
)
target_link_libraries(unit_tests PRIVATE rvcore rvsim_options GTest::gtest_main)
gtest_discover_tests(unit_tests)
```

Why it's shaped this way:

- `rvcore` carries C++20 and the include path as `PUBLIC` properties, so the simulator and the tests inherit both just by linking it.
- The warnings live on `rvsim_options` instead of in global flags, so they apply to your code only. Turning `-Werror` on for third-party code breaks builds for reasons you can't fix.
- `-fno-sanitize-recover=all` makes UBSan stop the program at the first error. Without it, UBSan prints a message and keeps going, and the test still passes.
- When you add a source or test file, add it to the matching `add_library` or `add_executable` list.

#### Step 0.5: Build and run

Use two build folders: debug with sanitizers for everyday work, and release for timing later.

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DRVSIM_SANITIZE=ON
cmake --build build/debug -j
ctest --test-dir build/debug --output-on-failure
./build/debug/sim/rvsim

cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j
```

From now on you only rerun the `cmake --build` and `ctest` lines. CMake reconfigures itself when a `CMakeLists.txt` changes; rerun the first line only to change options.

**Troubleshooting:** if every sanitizer run dies at startup with `AddressSanitizer:DEADLYSIGNAL` (during the build's test discovery step or under `ctest`), your kernel's address randomization is set higher than your GCC's sanitizer runtime supports. Lower it:

```bash
sudo sysctl vm.mmap_rnd_bits=28
echo 'vm.mmap_rnd_bits=28' | sudo tee /etc/sysctl.d/99-asan.conf   # keep it after reboot
```

#### Step 0.6: Prove the harness catches problems

Break the build on purpose four ways, confirm each failure, then undo it.

- [ ] T0.1 `ctest` reports `Smoke.VersionIsNotEmpty` as passed.
- [ ] T0.2 A failing test fails. Add this to `smoke_test.cpp`, run `ctest`, then delete it:

```cpp
TEST(Smoke, MustFail) {
  EXPECT_EQ(1, 2);
}
```

- [ ] T0.3 Warnings are errors. Add `int unused = 0;` inside `main()`, confirm the build fails, then remove it.
- [ ] T0.4 AddressSanitizer is active. Add this test (with `#include <vector>` at the top), confirm the debug run reports `heap-buffer-overflow`, then delete it:

```cpp
TEST(Smoke, AsanCatchesOverflow) {
  std::vector<int> v(4);
  volatile int i = 4;  // volatile keeps the compiler from flagging it early
  EXPECT_EQ(v.data()[i], 0);  // reads past the end on purpose
}
```

- [ ] T0.5 UBSan is active. Add this test (with `#include <limits>`), confirm the debug run reports a signed integer overflow, then delete it:

```cpp
TEST(Smoke, UbsanCatchesSignedOverflow) {
  volatile int one = 1;
  int x = std::numeric_limits<int>::max();
  x += one;  // signed overflow on purpose
  EXPECT_NE(x, 0);
}
```

#### Step 0.7: Formatting, editor support, and ignores

`.clang-format`

```yaml
BasedOnStyle: Google
ColumnLimit: 100
```

`.gitignore`

```
build/
compile_commands.json
*.elf
*.bin
```

clang-format handles layout only, not naming; your naming rules go in `DESIGN.md` (Step 0.10). Format all files with:

```bash
clang-format -i include/rvsim/*.hpp src/*.cpp sim/*.cpp tests/unit/*.cpp
```

If your editor uses clangd (VS Code, Neovim), link the compile database into the repository root so it finds your include paths and flags:

```bash
ln -s build/debug/compile_commands.json .
```

#### Step 0.8: Check the cross toolchain

```bash
riscv64-unknown-elf-gcc --version
riscv64-unknown-elf-gcc -print-multi-lib | grep rv32i
```

The second command should list an `rv32i/ilp32` entry. The real test is whether a 32-bit libgcc links, because Lab 8 needs it for multiplication and division:

```bash
mkdir -p /tmp/rvcheck && cd /tmp/rvcheck
cat > mul.c << 'EOF'
int mul(int a, int b) { return a * b; }
void _start(void) { mul(3, 4); for (;;) {} }
EOF
riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -nostdlib -Ttext=0x80000000 -Wl,-n \
    mul.c -lgcc -o mul.elf
riscv64-unknown-elf-objdump -d mul.elf | grep mulsi3
```

If it links and `objdump` shows `__mulsi3`, you're set. If you get "skipping incompatible", "cannot find -lgcc", or an ABI mismatch error, the package has no 32-bit libgcc. Use one of these instead, then rerun the check with the new prefix:

- The xPack build: download a release from github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack, unpack it, add its `bin` folder to `PATH`, and use the `riscv-none-elf-` prefix.
- Build riscv-gnu-toolchain for RV32I. It takes a while; install the prerequisites from its README first.

```bash
git clone https://github.com/riscv-collab/riscv-gnu-toolchain
cd riscv-gnu-toolchain
./configure --prefix="$HOME/opt/riscv32" --with-arch=rv32i --with-abi=ilp32
make -j"$(nproc)"
export PATH="$HOME/opt/riscv32/bin:$PATH"   # tools are named riscv32-unknown-elf-*
```

Write down the prefix you ended up with. The rest of this walkthrough uses `riscv64-unknown-elf-`, and your Lab 8 Makefile will keep the prefix in one variable.

#### Step 0.9: Your first RISC-V program

`tests/programs/lab0/first.S`

```asm
/* Lab 0: a few hand-written instructions to inspect with objdump and readelf.
   Lab 7 runs this same program; it should exit with code 12. */

        .text
        .globl _start
_start:
        addi    a0, zero, 5         # a0 = 5
        addi    a1, zero, 7         # a1 = 7
        add     a2, a0, a1          # a2 = 12
        sub     a3, a1, a0          # a3 = 2
        lui     a4, 0x12345         # a4 = 0x12345000
        beq     a0, a1, done        # not taken
        mv      a5, a2              # alias for addi a5, a2, 0
done:
        li      a7, 93              # exit syscall number
        mv      a0, a2              # exit code 12
        ecall
```

Build and inspect it:

```bash
cd tests/programs/lab0
riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -nostdlib -Ttext=0x80000000 -Wl,-n \
    first.S -o first.elf
riscv64-unknown-elf-objdump -d first.elf
riscv64-unknown-elf-objdump -d -M no-aliases first.elf
riscv64-unknown-elf-readelf -h first.elf
riscv64-unknown-elf-readelf -l first.elf
```

`-Ttext` sets the address the code is linked at. `-Wl,-n` stops the linker from page-aligning the segment. Without it, the segment starts a page lower (0x7ffff000) so it can also hold the ELF headers, and part of it falls outside your RAM.

What to look for:

- `objdump -d` shows `_start` at `80000000`, followed by each instruction and its hex word.
- With `-M no-aliases`, `li` and `mv` appear as `addi`. Your Lab 5 disassembler should print this form.
- `readelf -h` shows class ELF32, little endian, machine RISC-V, and entry point `0x80000000`.
- `readelf -l` shows a LOAD segment at `0x80000000` with a file size of `0x28` (ten 4-byte instructions). These are the fields your Lab 8 ELF loader reads. The RISCV_ATTRIBUTES entry isn't loadable, so your loader skips it.

#### Step 0.10: Start DESIGN.md

`docs/DESIGN.md`

```markdown
# Design decisions

One entry per decision: what I chose, then why. Labs that say "record it" add an entry here.

## D1. ISA: RV32I, little-endian
Why: matches my SystemVerilog RV32I core, so this model can check it later.

## D2. Memory map: RAM at 0x80000000, 1 MiB by default, size configurable
Why: same base as Spike, QEMU virt, and the riscv-tests linker script.
Programs are linked at the same address they are loaded at.

## D3. Halting: ECALL with a7 = 93 exits with the code in a0
Why: Linux and newlib put the syscall number in a7.

## D4. Naming: types in PascalCase, functions and variables in snake_case
Why: consistent with the hints. clang-format handles layout only.
```

#### Step 0.11: Commit

```bash
git init            # skip if the repository already exists
git add .
git commit -m "Lab 0: project skeleton, test harness, toolchain check"
git tag v0.0
```

**Done when**

- [ ] The debug and release builds finish with zero warnings.
- [ ] `ctest` passes in `build/debug`, and you watched T0.2 to T0.5 fail on purpose.
- [ ] `./build/debug/sim/rvsim` prints the version.
- [ ] The libgcc check links for rv32i/ilp32.
- [ ] You can point to every `first.elf` field your ELF loader will need.
- [ ] `DESIGN.md` has entries D1 to D4.

**C++ focus:** build targets and usage requirements, header and source split, namespaces, compiler flags.

---

## Phase 1: Architectural state

### Lab 1: Bit toolkit (S)

**Goal:** small, heavily tested helpers for bit fields and sign extension. Everything after this lab depends on them.

**Objectives**
- [ ] 1.1 Extract bits `hi` down to `lo` from a 32-bit value.
- [ ] 1.2 Test a single bit.
- [ ] 1.3 Sign-extend an N-bit value to 32 bits.
- [ ] 1.4 Make every helper `constexpr` and `noexcept`.
- [ ] 1.5 Define type aliases for a register value, a signed register value, an address, and a raw instruction. Use them everywhere from now on.

**Tests**
- [ ] T1.1 Compile-time checks with `static_assert` on a few known values.
- [ ] T1.2 Runtime tests: full-width extraction, single bits, bit 31, and sign extension of positive, negative, largest, and smallest values at widths 1, 5, 8, 12, 16, 20, and 32.

**Done when:** all tests pass in the sanitizer build.

**C++ focus:** `constexpr`, `static_assert`, fixed-width integer types, `using` aliases, `[[nodiscard]]`, integer promotion and shift rules.

### Lab 2: Memory and the bus (M)

**Goal:** one address space for instructions and data. The CPU only talks to a bus, the bus routes each access to a device, and memory is one of those devices.

**Objectives**
- [ ] 2.1 Abstract `Device` class: a size, plus reads and writes of 1, 2, and 4 bytes at an offset.
- [ ] 2.2 `Memory`, derived from `Device`: allocate the byte storage that holds the program's instructions and data.
- [ ] 2.3 Little-endian multi-byte reads and writes.
- [ ] 2.4 Bounds checking that throws a custom exception carrying the address, the width, and the access type (fetch, read, or write).
- [ ] 2.5 `Bus`: maps address ranges to devices, turns addresses into device offsets, rejects overlapping ranges, and offers instruction fetch as its own entry point next to data reads and writes.
- [ ] 2.6 A loader that copies a run of bytes to an address (files come in Lab 8).
- [ ] 2.7 A hex dump of an address range, for debugging.

**Tests**
- [ ] T2.1 Write a 32-bit value and read its bytes back one at a time: the least significant byte sits at the lowest address.
- [ ] T2.2 Round trips at every width at the first and last valid addresses.
- [ ] T2.3 Accesses that cross the end throw, including a 4-byte read that starts 1, 2, or 3 bytes before the end.
- [ ] T2.4 With two devices mapped, each address reaches the right device; unmapped addresses throw; overlapping ranges are rejected.
- [ ] T2.5 The loader places bytes at the right address.
- [ ] T2.6 A single typed test body covers the 8, 16, and 32-bit round trips.

**Done when:** all tests pass and you can explain why instruction fetch and data access in your design reach the same memory.

**C++ focus:** abstract classes, pure virtual functions, virtual destructors, templates layered on virtual functions, concepts, custom exceptions, `std::vector`, `std::span`, `std::unique_ptr` ownership, RAII.

### Lab 3: Register file and CPU state (S)

**Goal:** the 32 integer registers, the pc, and a way to compare and record CPU state.

**Objectives**
- [ ] 3.1 `RegisterFile` with 32 registers. x0 always reads 0 and ignores writes.
- [ ] 3.2 ABI name lookup (x10 is `a0`) for printing.
- [ ] 3.3 `ArchState` holding the pc and the registers, with equality comparison and a readable dump.
- [ ] 3.4 `CommitRecord` describing one retired instruction: pc, raw bits, the register write (if any), and the memory write (if any). You will use it for tracing, for comparing your two CPU models, and later for checking your RTL core.

**Tests**
- [ ] T3.1 Writes to x0 are ignored; x1 through x31 round-trip.
- [ ] T3.2 ABI names: x0 `zero`, x1 `ra`, x2 `sp`, x8 `s0`, x10 `a0`, x31 `t6`.
- [ ] T3.3 States that differ in one register compare unequal; identical states compare equal.

**Done when:** tests pass and a state dump prints all 32 registers as a readable table.

**C++ focus:** `std::array`, const correctness, defaulted `operator==`, `operator<<`, `std::string_view`, `std::optional`.

---

## Phase 2: Decode

### Lab 4: Instruction fields and immediates (M)

**Goal:** pull every field out of a raw instruction and build all five immediate types correctly.

**Objectives**
- [ ] 4.1 Read the RV32I chapter and the instruction listing table in the unprivileged spec. Draw the six formats (R, I, S, B, U, J) by hand.
- [ ] 4.2 An `enum class` for the major opcodes.
- [ ] 4.3 Field extractors: opcode, rd, rs1, rs2, funct3, funct7.
- [ ] 4.4 Immediate builders for the I, S, B, U, and J formats.
- [ ] 4.5 Generate test vectors with the cross assembler: one assembly file containing every RV32I instruction with chosen immediates (zero, positive, negative, and both range limits). Take the hex words from the disassembly.

**Tests**
- [ ] T4.1 Every immediate type: zero, small positive, small negative, largest positive, most negative.
- [ ] T4.2 B and J immediates always have bit 0 clear; include backward branches and jumps.
- [ ] T4.3 Every vector from 4.5 yields the fields and immediates the disassembly shows.

**Done when:** every vector decodes to the right fields.

**C++ focus:** `enum class` and underlying types, `constexpr`, test fixtures, table-driven tests.

### Lab 5: Instruction class hierarchy and decoder (L)

**Goal:** turn a raw instruction into an object that knows what it is and what it does in each stage.

**Objectives**
- [ ] 5.1 Abstract `Instruction` base class. It stores the pc, the raw bits, and the decoded fields. It declares the per-stage behavior (execute, memory access, write back), a disassembly method, and the queries the CPU will need later: which registers it reads, whether it writes rd, and whether it is a load, a control-flow instruction, or a system instruction.
- [ ] 5.2 Derived classes by category: `LoadInstruction`, `StoreInstruction`, `BranchInstruction`, `RegisterOpInstruction` (R-type), `ImmediateOpInstruction` (I-type arithmetic and shifts), `LuiInstruction`, `AuipcInstruction`, `JalInstruction`, `JalrInstruction`, `SystemInstruction` (ECALL, EBREAK), `FenceInstruction`, and `IllegalInstruction`.
- [ ] 5.3 Use class templates for the load family (width and signedness), the store family (width), and the branch family (comparison and signedness).
- [ ] 5.4 `Decoder`, a factory that returns an owning pointer to the right derived object for any 32-bit value, including illegal ones.
- [ ] 5.5 Disassembly: every instruction prints itself with its mnemonic and operands, using ABI register names.
- [ ] 5.6 Leave the stage behavior bodies empty for now; Lab 6 fills them in.

**Tests**
- [ ] T5.1 All 40 RV32I instructions decode to the right class and mnemonic (reuse the Lab 4 vectors).
- [ ] T5.2 Illegal encodings produce `IllegalInstruction`: all zeros, all ones, an unused opcode, JALR with a nonzero funct3, an R-type with a bad funct7, a branch with funct3 010 or 011, the RV64-only load and store variants (LD, LWU, SD), and a shift immediate with the wrong upper bits.
- [ ] T5.3 Disassembly of a sample of instructions matches the cross toolchain's objdump output (ignoring any spacing differences you decide not to care about).
- [ ] T5.4 Queries: LUI and JAL read no registers; stores and branches don't write rd; only R-type, stores, and branches read rs2.

**Done when:** every vector decodes to the right class and prints correctly.

**C++ focus:** pure virtual functions, `override` and `final`, virtual destructors, `std::unique_ptr` and `std::make_unique`, the factory and null object patterns, class templates derived from a non-template base, type traits.

---

## Phase 3: Execute

### Lab 6: Execution semantics (L)

**Goal:** every instruction computes the right result, branch decision, target, and memory effect.

**Objectives**
- [ ] 6.1 Decide where operand values and results live (inside the instruction object, or in separate stage structs) and record it in `DESIGN.md`.
- [ ] 6.2 An ALU for the ten R-type operations, reused by the I-type instructions.
- [ ] 6.3 Execute for arithmetic, logic, shifts, and comparisons.
- [ ] 6.4 Branch decisions and targets, JAL and JALR targets and link values, LUI and AUIPC results.
- [ ] 6.5 Memory access for loads (with sign or zero extension) and stores.
- [ ] 6.6 Write back, never to x0.
- [ ] 6.7 Detect a taken control transfer to an address that is not a multiple of 4, and represent it as a fault.

**Tests** (parameterized, one row per case)
- [ ] T6.1 ADD and SUB wrap around at 0x7FFFFFFF and 0x80000000.
- [ ] T6.2 SLT against SLTU with negative operands; SLTIU with an immediate of -1.
- [ ] T6.3 SRA against SRL on negative values; shift amounts 0, 1, and 31; an R-type shift whose rs2 has bits set above bit 4.
- [ ] T6.4 SRAI against SRLI.
- [ ] T6.5 All six branches, taken and not taken, including pairs where signed and unsigned comparisons disagree (-1 and 1).
- [ ] T6.6 JALR with an odd sum (bit 0 cleared), and JALR with rd equal to rs1.
- [ ] T6.7 JAL and JALR with rd = x0 write nothing.
- [ ] T6.8 LB and LH sign-extend 0x80 and 0x8000; LBU and LHU zero-extend; SB and SH change only their own bytes and leave neighbors alone.
- [ ] T6.9 AUIPC uses the instruction's own pc.
- [ ] T6.10 Loads and stores with negative offsets.
- [ ] T6.11 Misaligned targets fault only when the transfer is taken.

**Done when:** every RV32I instruction has at least one behavior test and all edge rows pass under the sanitizers.

**C++ focus:** `enum class` with exhaustive switches, lambdas, standard function objects, C++20 signed and unsigned conversion rules, parameterized tests.

---

## Phase 4: Unpipelined CPU

### Lab 7: The single-cycle 5-stage CPU (M)

**Goal:** a CPU that runs each instruction through all five stages before starting the next.

**Objectives**
- [ ] 7.1 Define the four inter-stage structs (IF/ID, ID/EX, EX/MEM, MEM/WB) now, even though nothing is pipelined yet. They become your pipeline registers in Phase 5.
- [ ] 7.2 `SingleCycleCpu` owns the register file and pc, and uses a bus it does not own.
- [ ] 7.3 One method per stage: fetch, decode, execute, memory access, write back.
- [ ] 7.4 `step()` runs the five stages for one instruction and returns its `CommitRecord`.
- [ ] 7.5 `run(limit)` loops until a stop reason: exit (with code), illegal instruction, memory fault, misaligned target, EBREAK, or instruction limit. Every stop reports the pc, the raw instruction, and the reason.
- [ ] 7.6 Faults are precise: a faulting instruction changes no register or memory, and the pc still points at it.
- [ ] 7.7 ECALL with a7 = 93 exits with the code in a0. Any other a7 value stops with a clear message.
- [ ] 7.8 Reset state: pc at the entry address, sp just below the top of memory, all other registers zero.
- [ ] 7.9 Counters: instructions executed and host time.
- [ ] 7.10 Optional: a multi-cycle variant that advances one stage per call, so every instruction takes five cycles. It gives you a CPI baseline of 5 for Phase 5.

**Tests** (small hand-assembled programs written into memory by the test)
- [ ] T7.1 Sum 1 to 10 into a0 and exit: exit code 55.
- [ ] T7.2 Store and load round trips at every width.
- [ ] T7.3 A call and return with JAL and JALR, with a stack push and pop.
- [ ] T7.4 Forward and backward branches.
- [ ] T7.5 An illegal instruction and a fetch outside memory stop with the right reason and pc, and leave state unchanged.
- [ ] T7.6 The instruction limit stops an infinite loop.
- [ ] T7.7 The Lab 0 program (`tests/programs/lab0/first.S`) exits with code 12.

**Done when:** all tests pass.

**C++ focus:** composition, references for non-owning links, `std::variant` and `std::visit` (or `std::optional`) for stop reasons, `std::chrono`, exception boundaries.

### Lab 8: Running real programs (L)

**Goal:** compile C and assembly with the cross toolchain and run the results.

**Objectives**
- [ ] 8.1 A startup file in assembly: `_start` sets up the stack if needed, calls `main`, and exits through ECALL with main's return value.
- [ ] 8.2 A linker script that places code and data in one RAM region starting at your memory base. The link address must equal the load address.
- [ ] 8.3 A build for test programs (a Makefile, or a separate CMake project with a toolchain file) using `-march=rv32i -mabi=ilp32`, freestanding, with no standard library.
- [ ] 8.4 The small runtime pieces the compiler calls on its own: `memset` and `memcpy`, plus multiply and divide helpers unless you link libgcc.
- [ ] 8.5 Load flat binaries (made with objcopy) first.
- [ ] 8.6 An ELF32 loader: validate the header (magic, 32-bit, little-endian, RISC-V), load each loadable segment, zero the rest of each segment's memory size, and start at the entry point.
- [ ] 8.7 A console device on the bus: writing a byte to its address prints a character. Add a tiny print routine to your C runtime.
- [ ] 8.8 Simulator command line: program path, instruction limit, trace switch, and a model switch (only one model exists for now). The process exit code equals the guest exit code.
- [ ] 8.9 A suite of self-checking programs where `main` returns 0 on success and a distinct nonzero code per failed check: Fibonacci (loop and recursive), bubble sort, string functions, GCD, shift-and-add multiply, xorshift random numbers, Collatz, and a "hello" that prints.
- [ ] 8.10 Build every program at `-O0` and `-O2`.

**Tests**
- [ ] T8.1 ELF loader unit tests on a small ELF checked into the repository: entry point, segment bytes, zeroed bss.
- [ ] T8.2 The loader rejects bad magic, 64-bit files, big-endian files, the wrong machine, and segments outside memory.
- [ ] T8.3 One CTest entry per program per optimization level, passing when the simulator exits with 0.

**Done when:** every program passes at both optimization levels and "hello" prints.

**C++ focus:** binary file I/O, `std::filesystem`, parsing binary formats field by field, exceptions with context, command-line parsing, process exit codes.

### Lab 9: Tracing and the official tests (M)

**Goal:** see exactly what the CPU did, and prove it against the official ISA tests.

**Objectives**
- [ ] 9.1 Instruction trace: one line per committed instruction with a count, pc, raw bits, disassembly, register write, and memory write.
- [ ] 9.2 The trace goes to any output stream (terminal or file), is off by default, and costs almost nothing when off.
- [ ] 9.3 A stable, machine-readable commit log (no counts, no disassembly) so two runs can be compared with `diff`.
- [ ] 9.4 Build riscv-tests (rv32ui) with your own minimal test environment so they run without CSRs or traps.
- [ ] 9.5 Register every applicable rv32ui test with CTest.
- [ ] 9.6 Optional: compare your commit log with Spike's for a program both can run.

**Tests**
- [ ] T9.1 All applicable rv32ui tests pass.
- [ ] T9.2 The trace of a short known program matches a checked-in expected file (golden file test).

**Done when:** all applicable rv32ui tests pass. Tag v0.1.

**C++ focus:** `std::ostream` references, stream formatting or `std::format`, callbacks with `std::function`, golden file tests.

---

## Phase 5: Pipeline

### Lab 10: Pipeline structure (L)

**Goal:** a cycle-level 5-stage pipeline with up to five instructions in flight. No hazard handling yet.

**Objectives**
- [ ] 10.1 A common `CpuModel` interface (run, step, state, stats, commit callback) implemented by both CPUs, so the simulator and the tests can use either one.
- [ ] 10.2 `PipelinedCpu` with the four pipeline registers from 7.1, each with a valid bit.
- [ ] 10.3 `tick()` advances one clock cycle: compute every stage's output from the current register contents, then update all registers together.
- [ ] 10.4 Commit happens only in WB. Exits, faults, and illegal instructions take effect only when they commit.
- [ ] 10.5 Reuse the Lab 6 instruction behavior. The pipeline only moves instructions between stages and calls their methods.
- [ ] 10.6 Pipeline diagram output: one row per cycle showing what occupies each stage.
- [ ] 10.7 Stats: cycles, committed instructions, CPI.
- [ ] 10.8 Decide how the single memory serves IF and MEM in the same cycle, and record the decision.
- [ ] 10.9 Write hazard-free test programs: no branches or jumps, and enough NOPs between each register write and any later read of that register.

**Tests**
- [ ] T10.1 N hazard-free instructions (including the final ECALL) take N + 4 cycles.
- [ ] T10.2 The first commit happens in cycle 5.
- [ ] T10.3 On every hazard-free program, the final state and the commit log equal the single-cycle model's.

**Done when:** T10.3 passes.

**C++ focus:** move-only types moving through pipeline registers, `std::exchange`, runtime polymorphism for the model interface, designated initializers.

### Lab 11: Data hazards (M)

**Goal:** correct results without NOP padding.

**Objectives**
- [ ] 11.1 Write back updates the register file before decode reads it in the same cycle.
- [ ] 11.2 Forwarding into EX from EX/MEM and from MEM/WB, newest value first, never for x0.
- [ ] 11.3 Forwarded values reach every consumer: ALU operands, branch comparisons, the JALR base, and store data.
- [ ] 11.4 Load-use detection: hold the pc and IF/ID for one cycle and send a bubble into ID/EX.
- [ ] 11.5 A register counts as used only if the instruction actually reads it.
- [ ] 11.6 Stats: stalls, and forwards by source.

**Tests** (branch-free programs)
- [ ] T11.1 A register read 1, 2, and 3 instructions after it is written.
- [ ] T11.2 Two consecutive writes to the same register, then a read: the newer value wins.
- [ ] T11.3 A write to x0 followed by a read of x0 reads 0.
- [ ] T11.4 A load followed immediately by a use: correct value and exactly one extra cycle.
- [ ] T11.5 A load followed by a use two instructions later: correct value and no stall.
- [ ] T11.6 A store whose base and data registers were just written, and a load whose base was just written.
- [ ] T11.7 Unit tests for the forwarding and stall decision functions on their own.
- [ ] T11.8 Every program above also runs on the single-cycle model with an identical commit log, and cycle counts equal N + 4 + stalls.

**Done when:** every branch-free hazard program matches the single-cycle model, including the expected cycle counts.

**C++ focus:** small pure functions that are easy to test, a shared test helper that runs one program on both models.

### Lab 12: Control hazards (M)

**Goal:** branches and jumps work in the pipeline.

**Objectives**
- [ ] 12.1 Resolve branches and jumps in EX. Predict not taken (fetch pc + 4).
- [ ] 12.2 On a redirect, discard the two instructions fetched after the branch and continue at the correct pc.
- [ ] 12.3 Define and document priorities for when a redirect, a stall, and the normal pc update meet in one cycle.
- [ ] 12.4 Wrong-path safety: a discarded instruction never writes a register or memory, never stops the simulation, and never reports an error. Fetch faults travel through the pipeline as data.
- [ ] 12.5 ECALL reads a0 and a7 from the register file when it commits.
- [ ] 12.6 Stats: redirects and flushed instructions.
- [ ] 12.7 Run the full Lab 8 program suite and the rv32ui tests on the pipeline.

**Tests**
- [ ] T12.1 A taken branch costs exactly 2 more cycles than the same branch not taken.
- [ ] T12.2 An exit ECALL and an illegal instruction placed right after a taken branch never take effect.
- [ ] T12.3 A jump stored in the last word of memory runs correctly even though the fetches after it fall outside memory.
- [ ] T12.4 Back-to-back branches; a branch whose target is another branch; a branch to itself, stopped by the instruction limit.
- [ ] T12.5 A branch and a JALR whose operands were just written (forwarding into control flow).
- [ ] T12.6 A JAL whose link register is read by the first instruction at its target.
- [ ] T12.7 Differential: every program and every rv32ui test produces an identical commit log on both models.

**Done when:** T12.7 passes. Tag v0.2.

**C++ focus:** invariants checked with assertions, `std::optional` for redirect requests.

---

## Phase 6: Branch prediction

### Lab 13: Predictor framework (M)

**Goal:** predict in IF, check in EX, recover on a mispredict.

**Objectives**
- [ ] 13.1 Abstract `BranchPredictor` interface: predict a direction for a pc, and train on the actual outcome.
- [ ] 13.2 Target prediction in IF: a direct-mapped branch target buffer (BTB), or pre-decoding direct branches in IF. Record which one you chose.
- [ ] 13.3 Carry each instruction's predicted next pc through the pipeline with it.
- [ ] 13.4 In EX, a mispredict is any difference between the actual next pc and the predicted next pc.
- [ ] 13.5 Recovery reuses the Lab 12 redirect path.
- [ ] 13.6 Static predictors: always not taken, always taken, and backward taken, forward not taken (BTFN).
- [ ] 13.7 Select the predictor by name on the command line through a small registry.
- [ ] 13.8 Stats: conditional branches, mispredictions, accuracy, MPKI, BTB hit rate.

**Tests**
- [ ] T13.1 Always-not-taken reproduces the Lab 12 cycle counts exactly.
- [ ] T13.2 For a loop with a known iteration count, each static predictor mispredicts exactly the number of times you computed on paper.
- [ ] T13.3 The differential check still passes with every predictor: prediction never changes architectural results.

**Done when:** T13.3 passes for every predictor.

**C++ focus:** the strategy pattern, a factory registry, `std::function`, dependency injection.

### Lab 14: Bimodal predictor and results (M)

**Goal:** the classic table of 2-bit counters, measured on your programs.

**Objectives**
- [ ] 14.1 `SaturatingCounter` class template with the bit width as a template parameter.
- [ ] 14.2 `BimodalPredictor` with a power-of-two table size chosen at construction, indexed from the pc.
- [ ] 14.3 Make the counters' initial state configurable.
- [ ] 14.4 Train on conditional branches only, in EX.
- [ ] 14.5 Sweep table sizes (for example 4 to 4096 entries) and predictors over the program suite. Write accuracy, MPKI, and CPI to a CSV.
- [ ] 14.6 Plot the results and write a short results section in the README.

**Tests**
- [ ] T14.1 Counters saturate at both ends, and the prediction is right in every state.
- [ ] T14.2 One not-taken outcome does not flip a strongly taken counter.
- [ ] T14.3 Two branches forced into the same entry of a tiny table interfere; with a larger table they don't.
- [ ] T14.4 A table size that is not a power of two is rejected.
- [ ] T14.5 A strictly alternating branch is mispredicted at least half the time.
- [ ] T14.6 The differential check passes.

**Done when:** the results table and plot exist. Tag v0.3.

**C++ focus:** non-type template parameters, `static_assert`, the `<bit>` header, typed tests.

---

## Phase 7: Caches

### Lab 15: Cache model (L)

**Goal:** a configurable cache hierarchy that counts hits and misses without changing any data.

**Objectives**
- [ ] 15.1 `Cache` with a size, block size, and associativity (all powers of two) that splits addresses into tag, index, and offset.
- [ ] 15.2 A replacement policy interface with LRU, plus FIFO or random for comparison.
- [ ] 15.3 Hit, miss, and eviction stats for reads and writes.
- [ ] 15.4 A memory hierarchy between the CPU and the bus: fetches go through an L1 instruction cache, loads and stores through an L1 data cache, and both fall back to a shared L2.
- [ ] 15.5 Caches track metadata only. Data always comes from memory, so a cache bug can never corrupt results. Device addresses (the console) bypass the caches.
- [ ] 15.6 Cache geometry comes from command-line options, and every run prints its effective configuration.

**Tests**
- [ ] T15.1 Tag, index, and offset for several geometries.
- [ ] T15.2 In a direct-mapped cache, two alternating addresses that share an index always miss.
- [ ] T15.3 A 2-way cache removes that conflict.
- [ ] T15.4 A known access sequence evicts the blocks LRU says it should.
- [ ] T15.5 The differential check passes.

**Done when:** miss rates for the program suite are recorded for several geometries.

**C++ focus:** composition, policy classes as templates and as virtual interfaces (try both), `std::list` with `std::unordered_map`, the decorator pattern.

### Lab 16: Cache timing and results (M)

**Goal:** cache misses cost cycles in the pipeline.

**Objectives**
- [ ] 16.1 Give each cache level and main memory a latency in cycles.
- [ ] 16.2 IF waits on an instruction cache miss. MEM waits on a data cache miss and holds every stage behind it.
- [ ] 16.3 Decide what wrong-path fetches do to the caches and to stalls, and record it.
- [ ] 16.4 Stats: stall cycles by cause (load-use, fetch miss, data miss), average memory access time, and CPI.
- [ ] 16.5 Sweep cache sizes and associativity, and add CPI against cache size to the README.

**Tests**
- [ ] T16.1 A single miss with latency L adds exactly L cycles.
- [ ] T16.2 A data cache miss in MEM while a taken branch sits in EX: the branch is not lost and redirects exactly once.
- [ ] T16.3 The differential check passes with timing enabled.

**Done when:** the CPI results are recorded. Tag v0.4.

**C++ focus:** configuration structs, strong types for cycle counts, stats collections.

---

## Phase 8: Extensions (pick any)

- E1 gshare predictor with a global history register; compare it with bimodal on the alternating branch from T14.5.
- E2 Return address stack for function returns.
- E3 Tournament predictor (bimodal and gshare with a chooser table).
- E4 RV32M: multiply and divide, with the spec's rules for division by zero and overflow; drop the libgcc helpers.
- E5 Zicsr and machine-mode traps, enough to run the unmodified riscv-tests environment (which ends a test by writing to the `tohost` symbol).
- E6 Reference model for your SystemVerilog RV32I core: a C-callable step-and-commit API that Verilator can call through DPI-C to check every instruction the RTL retires.
- E7 Pipeline visualization in Konata.
- E8 Simulator speed: profile, then add a decoded-instruction cache and measure host instructions per second before and after.
- E9 Transient-execution playground: lengthen the window between prediction and resolution, run a bounds-check-bypass style program, and show the secret-dependent footprint that wrong-path instructions leave in the caches.
- E10 RV64I, with the register width as a template parameter.
- E11 The RISC-V architecture tests (riscv-arch-test) through RISCOF, with your model as the device under test.

---

## Decision log starter

| Decision | Where |
|---|---|
| RV32I, little-endian, memory base and size | 0.10 |
| Where operand values and results live | 6.1 |
| How a program halts (ECALL with a7 = 93) | 7.7 |
| How the pipeline updates its registers | 10.3 |
| How one memory serves IF and MEM in the same cycle | 10.8 |
| Priorities for redirect, stall, and normal pc update | 12.3 |
| BTB or pre-decode for targets | 13.2 |
| When predictors train; counter initial state | 13.8, 14.3 |
| What wrong-path accesses do to the caches | 16.3 |

## C++ coverage

| Feature | Labs |
|---|---|
| Abstract classes, virtual destructors, `override`, `final` | 2, 5, 10, 13, 15 |
| Class templates, templates deriving from a non-template base | 5, 14, 15 |
| Function templates and concepts | 2 |
| `constexpr`, `static_assert`, non-type template parameters, `<bit>` | 1, 14 |
| `std::unique_ptr` and move-only types | 2, 5, 10 |
| `enum class` and exhaustive switches | 4, 6 |
| `std::optional`, `std::variant`, `std::visit` | 3, 7, 12 |
| `std::array`, `std::vector`, `std::span`, `std::string_view` | 2, 3 |
| Defaulted comparisons, `operator<<` | 3 |
| Exceptions and custom exception types | 2, 7, 8 |
| Lambdas, function objects, `std::function` | 5, 6, 9, 13 |
| Patterns: factory, null object, observer, strategy, decorator | 5, 9, 13, 15 |
| Binary file I/O, `std::filesystem` | 8 |
| Testing: fixtures, parameterized, typed, golden file, differential | all |

## References

- RISC-V Unprivileged ISA specification: the RV32I chapter and the instruction set listings (releases at github.com/riscv/riscv-isa-manual).
- Patterson and Hennessy, *Computer Organization and Design, RISC-V Edition*: chapter 4 (processor, pipelining, hazards, branch prediction) and chapter 5 (caches).
- Harris and Harris, *Digital Design and Computer Architecture, RISC-V Edition*: chapters 7 and 8.
- riscv-tests: github.com/riscv-software-src/riscv-tests
- fmash16, "Writing a simple RISC-V emulator in plain C" (RV64, in C; see the notes at the top of `hints.md`), and the Rust book it follows at book.rvemu.app.
- Scott McFarling, "Combining Branch Predictors" (DEC WRL Technical Note TN-36, 1993): bimodal and gshare.
- GoogleTest documentation on parameterized and typed tests.
