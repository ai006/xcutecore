# RV32I CPU Model in C++: Lab Path

You are building a small computer in C++: a RISC-V RV32I CPU and a memory, connected by a bus, with instructions and data sharing one address space (Von Neumann). The CPU starts unpipelined: each instruction goes through fetch, decode, execute, memory, and write back before the next one starts. Once it runs real programs and passes the official tests, you turn it into a cycle-level 5-stage pipeline, then add branch prediction and caches.

Every lab lists objectives, tests, a "done when" check, and the C++ it exercises. Lab 0 is the exception: a step-by-step setup walkthrough with code. Objectives are numbered by lab (2.3 is Lab 2, objective 3), and the hints use the same numbers. Each objective says what to build, what goes in, what comes out, and gives an example; objectives that build a class list the members it needs. The hints cover how to build it. Lab 2 has its own walkthrough, `hints_lab2.md`, and later labs will get one each as they are rewritten; `hints.md` keeps the general notes and the hints for labs that don't have their own file yet. Try each objective before opening its hint. Names of functions, classes, files, and flags are suggestions: rename them freely, but keep the inputs and outputs.

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

## How the pieces fit

What owns or uses what, and which lab builds it:

```
rvsim (the program you run)                         sim/main.cpp, Lab 8
└── CPU: SingleCycleCpu or PipelinedCpu             Lab 7, Labs 10 to 12
    ├── RegisterFile, ArchState, CommitRecord       Lab 3
    ├── decode() and the Instruction objects        Labs 5, 6
    │   └── field extractors, immediate builders    Lab 4
    │       └── bit helpers, type aliases           Lab 1
    ├── BranchPredictor, BTB                        Labs 13, 14
    └── MemoryHierarchy (caches)                    Labs 15, 16
        └── Bus                                     Lab 2
            ├── Memory (instructions and data)      Lab 2
            └── ConsoleDevice                       Lab 8
```

Until Lab 15 the CPU talks to the `Bus` directly.

The life of one instruction on the single-cycle CPU, using `add a2, a0, a1` at 0x80000008 from your Lab 0 program:

1. **Fetch:** the CPU asks the bus for 4 bytes at pc 0x80000008 and gets the `Word` 0x00B50633. (Labs 2, 7)
2. **Decode:** `decode()` uses the bit helpers to cut the word into fields (opcode 0x33, rd 12, rs1 10, rs2 11, funct3 0, funct7 0) and returns an instruction object for ADD. The CPU reads a0 = 5 and a1 = 7 from the register file and hands them to it. (Labs 1, 4, 5)
3. **Execute:** the object computes 5 + 7 = 12 and sets its next pc to 0x8000000C. (Lab 6)
4. **Memory:** nothing for ADD. A load or store would use the bus here. (Labs 2, 6)
5. **Write back:** 12 goes into a2, and `step()` returns a `CommitRecord`: pc 0x80000008, word 0x00B50633, x12 = 12. (Labs 3, 7)

The pipeline (Labs 10 to 12) runs the same five steps, with up to five instructions in different steps at once.

## Where the files end up

Every file the core labs (0 to 16) leave in the repository, and the lab that creates it. It grows out of the Step 0.2 layout, with `scripts/`, `results/`, and `third_party/` added along the way. Like every name in these labs, the file names are suggestions. Not shown: build output (`build/`, including the program builds under `tests/programs/build/`), the `compile_commands.json` link, and anything the Phase 8 extensions add.

```
.
├── CMakeLists.txt                     Lab 0
├── README.md                          Lab 0; results added in Labs 14 and 16
├── .clang-format                      Lab 0
├── .gitignore                         Lab 0
├── docs/
│   ├── DESIGN.md                      Lab 0; grows with every decision you record
│   ├── labs.md
│   ├── hints.md                       general notes, plus labs without their own file
│   ├── hints_lab2.md                  Lab 2
│   └── hints_lab3.md ...              one per lab as they are written
├── include/
│   └── rvsim/
│       ├── version.hpp                Lab 0
│       ├── types.hpp                  Lab 1
│       ├── bits.hpp                   Lab 1
│       ├── device.hpp                 Lab 2
│       ├── memory_fault.hpp           Lab 2
│       ├── memory.hpp                 Lab 2
│       ├── bus.hpp                    Lab 2
│       ├── loader.hpp                 Lab 2 (load_bytes), Lab 8 (load_binary, load_elf)
│       ├── hex_dump.hpp               Lab 2
│       ├── register_file.hpp          Lab 3
│       ├── arch_state.hpp             Lab 3
│       ├── commit_record.hpp          Lab 3
│       ├── fields.hpp                 Lab 4
│       ├── instruction.hpp            Lab 5
│       ├── instructions.hpp           Labs 5, 6
│       ├── decode.hpp                 Lab 5
│       ├── alu.hpp                    Lab 6
│       ├── stages.hpp                 Lab 7
│       ├── stop_reason.hpp            Lab 7
│       ├── single_cycle_cpu.hpp       Lab 7
│       ├── console_device.hpp         Lab 8
│       ├── trace.hpp                  Lab 9
│       ├── cpu_model.hpp              Lab 10
│       ├── pipelined_cpu.hpp          Labs 10 to 12
│       ├── hazards.hpp                Lab 11
│       ├── branch_predictor.hpp       Lab 13
│       ├── btb.hpp                    Lab 13
│       ├── saturating_counter.hpp     Lab 14
│       ├── bimodal_predictor.hpp      Lab 14
│       ├── cache.hpp                  Lab 15
│       ├── replacement_policy.hpp     Lab 15
│       └── memory_hierarchy.hpp       Labs 15, 16
├── src/
│   ├── CMakeLists.txt                 Lab 0; lists every .cpp
│   ├── version.cpp                    Lab 0
│   ├── memory_fault.cpp               Lab 2
│   ├── memory.cpp                     Lab 2
│   ├── bus.cpp                        Lab 2
│   ├── loader.cpp                     Labs 2, 8
│   ├── hex_dump.cpp                   Lab 2
│   ├── register_file.cpp              Lab 3
│   ├── arch_state.cpp                 Lab 3
│   ├── instruction.cpp                Lab 5
│   ├── instructions.cpp               Labs 5, 6
│   ├── decode.cpp                     Lab 5
│   ├── alu.cpp                        Lab 6
│   ├── stop_reason.cpp                Lab 7
│   ├── single_cycle_cpu.cpp           Lab 7
│   ├── console_device.cpp             Lab 8
│   ├── trace.cpp                      Lab 9
│   ├── pipelined_cpu.cpp              Labs 10 to 12
│   ├── hazards.cpp                    Lab 11
│   ├── branch_predictor.cpp           Lab 13 (static predictors, make_predictor)
│   ├── btb.cpp                        Lab 13
│   ├── bimodal_predictor.cpp          Lab 14
│   ├── cache.cpp                      Lab 15
│   └── memory_hierarchy.cpp           Labs 15, 16
├── sim/
│   ├── CMakeLists.txt                 Lab 0
│   ├── main.cpp                       Lab 0; grows in Labs 8, 9, 10, 13, 15
│   ├── options.hpp                    Lab 8 (command-line parsing)
│   └── options.cpp                    Lab 8
├── tests/
│   ├── CMakeLists.txt                 Lab 0; lists every test file and CTest entry
│   ├── unit/
│   │   ├── smoke_test.cpp             Lab 0
│   │   ├── bits_test.cpp              Lab 1
│   │   ├── memory_test.cpp            Lab 2
│   │   ├── bus_test.cpp               Lab 2
│   │   ├── loader_test.cpp            Labs 2, 8
│   │   ├── register_file_test.cpp     Lab 3
│   │   ├── arch_state_test.cpp        Lab 3
│   │   ├── commit_record_test.cpp     Lab 3
│   │   ├── fields_test.cpp            Lab 4
│   │   ├── decode_test.cpp            Lab 5
│   │   ├── alu_test.cpp               Lab 6
│   │   ├── execute_test.cpp           Lab 6
│   │   ├── test_helpers.hpp           Labs 7, 11 (load words, run on one or both CPUs)
│   │   ├── single_cycle_cpu_test.cpp  Lab 7
│   │   ├── console_device_test.cpp    Lab 8
│   │   ├── trace_test.cpp             Lab 9
│   │   ├── pipeline_test.cpp          Lab 10
│   │   ├── hazards_test.cpp           Lab 11 (the decision functions)
│   │   ├── data_hazard_test.cpp       Lab 11 (the programs)
│   │   ├── control_hazard_test.cpp    Lab 12
│   │   ├── differential_test.cpp      Lab 12
│   │   ├── predictor_test.cpp         Lab 13
│   │   ├── bimodal_test.cpp           Lab 14
│   │   ├── cache_test.cpp             Lab 15
│   │   └── cache_timing_test.cpp      Lab 16
│   ├── programs/
│   │   ├── Makefile                   Lab 8
│   │   ├── link.ld                    Lab 8
│   │   ├── runtime/
│   │   │   ├── crt0.S                 Lab 8
│   │   │   ├── runtime.c              Lab 8
│   │   │   └── runtime.h              Lab 8
│   │   ├── lab0/
│   │   │   └── first.S                Lab 0
│   │   ├── lab4/
│   │   │   └── vectors.S              Lab 4
│   │   └── suite/                     Lab 8, one self-checking program per file
│   │       ├── fib_loop.c
│   │       ├── fib_recursive.c
│   │       ├── bubble_sort.c
│   │       ├── strings.c
│   │       ├── gcd.c
│   │       ├── multiply.c
│   │       ├── xorshift.c
│   │       ├── collatz.c
│   │       └── hello.c
│   ├── data/
│   │   └── first.elf                  Lab 8 (T8.1)
│   ├── riscv-tests-env/
│   │   ├── riscv_test.h               Lab 9
│   │   ├── link.ld                    Lab 9
│   │   └── Makefile                   Lab 9 (builds the rv32ui tests)
│   └── golden/
│       └── first.trace                Lab 9 (T9.2)
├── scripts/
│   ├── sweep.py                       Lab 14; reused in Lab 16
│   └── plot.py                        Lab 14; reused in Lab 16
├── results/
│   ├── predictor_sweep.csv            Lab 14
│   ├── mpki_vs_entries.png            Lab 14
│   ├── cache_sweep.csv                Lab 16
│   └── cpi_vs_cache_size.png          Lab 16
└── third_party/
    └── riscv-tests/                   Lab 9 (git submodule)
```

One catch for later: the Lab 0 `.gitignore` ignores `*.elf`, but Lab 8 checks `tests/data/first.elf` into the repository. Add a `!tests/data/*.elf` line to `.gitignore` when you get there.

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

- [X] The debug and release builds finish with zero warnings.
- [X] `ctest` passes in `build/debug`, and you watched T0.2 to T0.5 fail on purpose.
- [X] `./build/debug/sim/rvsim` prints the version.
- [X] The libgcc check links for rv32i/ilp32.
- [X] You can point to every `first.elf` field your ELF loader will need.
- [X] `DESIGN.md` has entries D1 to D4.

**C++ focus:** build targets and usage requirements, header and source split, namespaces, compiler flags.

---

## Phase 1: Architectural state

### Lab 1: Bit toolkit (S)

**Goal:** small, heavily tested helpers that pull bit fields out of a 32-bit word and sign-extend them. Everything after this lab depends on them.

**Where this fits:** every instruction your CPU runs arrives as one 32-bit number. In Lab 4 the decoder uses these helpers to cut that number into fields (opcode, rd, rs1, and so on) and to rebuild immediates. After decode, nothing else in the CPU touches raw bits.

All helpers work on plain integers: a `Word` goes in, and a `Word` or a `bool` comes out.

**Files:** `include/rvsim/types.hpp` for the aliases and `include/rvsim/bits.hpp` for the helpers. Both can be header-only, since `constexpr` functions live in headers. Put the tests in `tests/unit/bits_test.cpp` and add it to the `unit_tests` source list.

**Objectives**
- [X] 1.1 **Type aliases** in `types.hpp`, inside the `rvsim` namespace:
  - `Word`: unsigned 32-bit. Register values and raw instructions.
  - `SWord`: signed 32-bit. Used only where an operation needs signed meaning.
  - `Addr`: unsigned 32-bit memory address.
  - `RegIndex`: a register number, 0 to 31.
- [X] 1.2 **`bits(value, hi, lo)`**. Takes a `Word` and two bit positions with 0 ≤ `lo` ≤ `hi` ≤ 31. Returns the bits from `hi` down to `lo` as a `Word`, moved down so that bit `lo` lands at bit 0. Example: `bits(0x00B50633, 11, 7)` returns 12. That word is `add a2, a0, a1` from your Lab 0 program, and bits 11 to 7 are its rd field (a2 is x12).
- [X] 1.3 **`bit(value, n)`**. Takes a `Word` and a position from 0 to 31. Returns `true` if that bit is 1. Example: `bit(0x40A586B3, 30)` is `true`; that bit is the one difference between `sub a3, a1, a0` and an ADD.
- [X] 1.4 **`sign_extend(value, width)`**. Takes a `Word` whose low `width` bits (1 to 32) hold a two's complement number, and returns that number as a 32-bit `Word` by copying bit `width - 1` into every bit above it. Bits above `width` in the input are ignored. Example: the 12-bit field 0xFFF means -1, so `sign_extend(0xFFF, 12)` returns 0xFFFFFFFF, while `sign_extend(0x7FF, 12)` returns 0x000007FF (2047). The result stays a `Word`: you will add it to register values with unsigned arithmetic, which wraps correctly.
- [X] 1.5 Make 1.2 to 1.4 `constexpr`, `noexcept`, and `[[nodiscard]]`, and check their preconditions with `assert`. A bad bit position is a bug in your simulator, not in the guest program.
- [X] 1.6 Optional: **`to_binary_string(value)`** for debugging. Returns a `std::string` with all 32 bits, most significant first, in groups of four. Example: 0x00B50633 becomes `0000 0000 1011 0101 0000 0110 0011 0011`. Handy in test failure messages and when you check field boundaries in Lab 4.

**Worked examples** (your first test cases)

| Call | Returns | Why |
|---|---|---|
| `bits(0x00B50633, 6, 0)` | `0x33` | opcode of `add a2, a0, a1` |
| `bits(0x00B50633, 11, 7)` | `12` | rd: a2 is x12 |
| `bits(0x00B50633, 19, 15)` | `10` | rs1: a0 is x10 |
| `bits(0x00B50633, 24, 20)` | `11` | rs2: a1 is x11 |
| `bits(0xDEADBEEF, 31, 28)` | `0xD` | top four bits |
| `bits(0xDEADBEEF, 31, 0)` | `0xDEADBEEF` | full width |
| `bit(0x40A586B3, 30)` | `true` | `sub a3, a1, a0` |
| `bit(0x00B50633, 30)` | `false` | `add a2, a0, a1` |
| `sign_extend(0x7FF, 12)` | `0x000007FF` | largest 12-bit value, 2047 |
| `sign_extend(0x800, 12)` | `0xFFFFF800` | most negative 12-bit value, -2048 |
| `sign_extend(0xFFF, 12)` | `0xFFFFFFFF` | -1, the immediate of `addi a0, zero, -1` (0xFFF00513) |
| `sign_extend(0xABC00FFF, 12)` | `0xFFFFFFFF` | bits above bit 11 are ignored |
| `sign_extend(0x1, 1)` | `0xFFFFFFFF` | a 1-bit field holding 1 is -1 |
| `sign_extend(0x12345678, 32)` | `0x12345678` | full width: unchanged |

**Tests**
- [X] T1.1 Compile-time checks: turn a few rows of the table into `static_assert`s.
- [X] T1.2 Runtime tests: every row of the table; bits 0 and 31 of 0x80000001; `bits` with `hi` equal to `lo`; and `sign_extend` at widths 1, 5, 8, 12, 13, 16, 20, 21, and 32, using the largest positive and the most negative value at each width. (13 and 21 are the widths of the B and J immediates.)
- [X] T1.3 Optional: a death test showing that `bits(x, 3, 7)` (hi below lo) stops the debug build at the assert.

**Done when:** all tests pass in the sanitizer build.

**C++ focus:** `constexpr`, `static_assert`, `assert`, fixed-width integer types, `using` aliases, `[[nodiscard]]`, integer promotion and shift rules.

### Lab 2: Memory and the bus (M)

**Goal:** one address space for instructions and data. The CPU only talks to a bus, the bus routes each access to a device, and memory is one of those devices.

**Where this fits:** in Lab 7 the fetch stage asks the bus for 4 bytes at the pc, and the memory stage asks it to load or store data. Both reach the same `Memory` object, which is what makes this a Von Neumann machine. In Lab 8 you map a console device next to memory, and in Lab 15 caches sit in front of the bus.

```
CPU (Lab 7)
 │   fetch(addr)    read(addr, width)    write(addr, width, value)
 ▼
Bus: finds the device that holds addr, subtracts that device's base
 ├── Memory at 0x80000000, 1 MiB        bus address 0x80000010 arrives as offset 0x10
 └── ConsoleDevice at 0x10000000        Lab 8
```

**Rules for every access**
- An access has an address, a width of 1, 2, or 4 bytes, and a value that travels as a `Word`. On the bus the address is a bus address; inside a device it is an offset from the device's first byte.
- Reads return the bytes zero-extended to a `Word`. Writes store only the low `width` bytes of the value. Sign extension for LB and LH happens later, in the load instruction (Lab 6), not here.
- Types: addresses and offsets are `Addr`, values are `Word`, and byte counts (sizes, widths, lengths) are `std::size_t`. `hints_lab2.md` explains why a size is neither `int` nor `Word`.

**Hints:** `hints_lab2.md` walks through every objective with scaffolds and gotchas.

**Files:** add each `.cpp` to `rvcore` in `src/CMakeLists.txt` and each test file to `unit_tests` in `tests/CMakeLists.txt`.

| File | What goes in it | Objectives |
|---|---|---|
| `include/rvsim/device.hpp` | `Device` (header only) | 2.1 |
| `include/rvsim/memory.hpp`, `src/memory.cpp` | `Memory` | 2.2, 2.3, 2.4 |
| `include/rvsim/memory_fault.hpp`, `src/memory_fault.cpp` | `AccessType`, `MemoryFault` | 2.4 |
| `include/rvsim/bus.hpp`, `src/bus.cpp` | `Bus`, `BusValue` | 2.5, 2.6 |
| `include/rvsim/loader.hpp`, `src/loader.cpp` | `load_bytes` | 2.7 |
| `include/rvsim/hex_dump.hpp`, `src/hex_dump.cpp` | `hex_dump` | 2.8 |
| `tests/unit/memory_test.cpp` | T2.1 to T2.4 | |
| `tests/unit/bus_test.cpp` | T2.5, T2.6 | |
| `tests/unit/loader_test.cpp` | T2.7, T2.8 | |

**Objectives**
- [X] 2.1 **Abstract class `Device`**: anything the bus can map, seen as a block of bytes. It has no data members. Its public interface:

  | Member | Takes | Returns | Kind |
  |---|---|---|---|
  | destructor | | | virtual, defaulted |
  | `size()` | nothing | `std::size_t`: how many bytes the device covers | pure virtual, `const` |
  | `read(offset, width)` | `Addr offset`, `std::size_t width` | `Word`: the bytes, zero-extended | pure virtual |
  | `write(offset, width, value)` | `Addr offset`, `std::size_t width`, `Word value` | nothing | pure virtual |

  `offset` counts from the device's first byte (offset 0), never a bus address; turning bus addresses into offsets is the bus's job (2.5). Because the class has pure virtual functions, `Device d;` must not compile. Example: a 1 MiB `Memory` reports `size()` 0x100000, and a 4-byte bus read at 0x80000010 reaches it as `read(0x10, 4)`.
- [X] 2.2 **`Memory`**, publicly derived from `Device`: the RAM that holds the program's instructions and data.
  - Constructor: takes the size in bytes as a `std::size_t` and allocates that many bytes, all zero. Mark it `explicit`.
  - Storage: a `std::vector<std::uint8_t>` data member, one element per byte.
  - Overrides `size()`, `read()`, and `write()`, each marked `override`.
  - In this objective `read` and `write` handle width 1 only; 2.3 adds 2 and 4. `read(offset, 1)` returns the byte at `offset`, and `write(offset, 1, value)` stores the low 8 bits of `value`.

  Example: a `Memory` built with 16 has `size()` 16 and reads 0 at every offset. After `write(3, 1, 0xAB)`, `read(3, 1)` returns 0xAB. `write(4, 1, 0x1FF)` stores 0xFF.
- [ ] 2.3 **Little-endian 2- and 4-byte access** in `Memory::read` and `Memory::write`: the byte at the lowest offset is the least significant. Accesses don't need to be aligned (Lab 9's `rv32ui-ma_data` test runs misaligned loads and stores). Examples: writing 0x12345678 as 4 bytes at offset 0 stores the bytes 78 56 34 12 at offsets 0 to 3. A 2-byte read at offset 1 then returns 0x3456, and a 1-byte read at offset 3 returns 0x12. A 2-byte write of 0xABCD1234 at offset 8 stores 34 12 at offsets 8 and 9 and leaves offset 10 alone.
- [ ] 2.4 **`MemoryFault`** and bounds checks.
  - `enum class AccessType` with `Fetch`, `Read`, and `Write`.
  - `MemoryFault`, derived from `std::runtime_error`. Its constructor takes an `Addr`, a `std::size_t` width, and an `AccessType`; the getters `addr()`, `width()`, and `type()` return them. `what()` returns a message such as `memory fault: 4-byte read at 0x0000000d`.
  - `Memory::read` throws `MemoryFault` with `AccessType::Read`, and `Memory::write` with `AccessType::Write`, when the access doesn't fit entirely inside the memory. The fault carries the offset, because a memory only knows offsets and can't tell a fetch from a data read. The bus (2.5) reports bus addresses and fetches.

  Example: in a 16-byte memory, a 4-byte read at offset 12 works; one at offset 13 throws, with `addr()` 13, `width()` 4, and `type()` `AccessType::Read`. A 1-byte write at offset 16 throws with `AccessType::Write`.
- [ ] 2.5 **`Bus`**: owns every device and turns bus addresses into device offsets. Public members:

  | Member | Takes | Returns | Does |
  |---|---|---|---|
  | `map(base, device)` | `Addr base`, `std::unique_ptr<Device> device` | nothing | Takes ownership and places the device at `base`, covering `base` to `base + size() - 1`. Throws `std::invalid_argument` if that range overlaps a mapped device or runs past 0xFFFFFFFF. |
  | `fetch(addr)` | `Addr` | `Word` | Instruction fetch, always 4 bytes. Its faults say `AccessType::Fetch`. |
  | `read(addr, width)` | `Addr`, `std::size_t` | `Word` | Data read. Its faults say `AccessType::Read`. |
  | `write(addr, width, value)` | `Addr`, `std::size_t`, `Word` | nothing | Data write. Its faults say `AccessType::Write`. |

  Each access finds the device whose range holds `addr`, subtracts that device's base, and forwards the offset. If no device holds `addr`, or the access doesn't fit entirely inside the device that does, the bus throws `MemoryFault` carrying the bus address. Example, with 1 MiB of memory at 0x80000000: address 0x80000010 reaches memory offset 0x10, 0x800FFFFC is the last valid word, a 4-byte read at 0x800FFFFE throws because it runs off the end, and both 0x80100000 and 0x7FFFFFFC throw. Mapping another device at 0x800FF000 throws `std::invalid_argument`; one at 0x80100000 is fine, because touching is not overlapping.
- [ ] 2.6 **Typed access** on the `Bus`, for when the width is known at compile time.
  - A concept `BusValue` that accepts exactly `std::uint8_t`, `std::uint16_t`, and `std::uint32_t`.
  - `read<T>(addr)` returns a `T`, and `write<T>(addr, value)` takes a `T`. Both are member function templates constrained with `BusValue`. The width is `sizeof(T)`, and both forward to the 2.5 functions.

  Examples: after `bus.write<std::uint32_t>(0x80000000, 0x12345678)`, `bus.read<std::uint16_t>(0x80000002)` returns 0x1234. `bus.write(0x80000004, std::uint8_t{0xFF})` writes one byte, with `T` deduced from the value. `bus.write(0x80000004, 0xFF)` (an `int`) and `bus.read<std::uint64_t>(0x80000000)` must not compile.
- [ ] 2.7 **Loader**: a free function `load_bytes(bus, start, bytes)` that takes a `Bus&`, an `Addr`, and a `std::span<const std::uint8_t>`, writes the bytes one at a time starting at `start`, and returns nothing. A byte that doesn't fit throws the bus's `MemoryFault`. Example: loading the bytes 13 05 50 00 at 0x80000000 and then fetching at 0x80000000 returns 0x00500513, the first instruction of your Lab 0 program (`addi a0, zero, 5`). Loading files comes in Lab 8.
- [ ] 2.8 **Hex dump**: a free function `hex_dump(bus, start, length)` that takes a `Bus&`, an `Addr`, and a `std::size_t`, and returns a `std::string`. One line per 16 bytes: the address of the line's first byte as 8 hex digits, a colon, then each byte as a space and two hex digits, then a newline. Hex is lowercase, and the last line may be shorter. Example: after loading the first four words of `first.S` at 0x80000000, `hex_dump(bus, 0x80000000, 16)` returns `"80000000: 13 05 50 00 93 05 70 00 33 06 b5 00 b3 86 a5 40\n"`.

**Tests** (one group per objective)
- [X] T2.1 Compile-time checks that `Device` is abstract and has a virtual destructor: `std::is_abstract_v` and `std::has_virtual_destructor_v` inside `static_assert`s.
- [X] T2.2 A new 16-byte memory has `size()` 16 and reads 0 at every offset. A 1-byte round trip, and a 1-byte write of 0x1FF reads back 0xFF.
- [ ] T2.3 Write a 32-bit value and read its bytes back one at a time: the least significant byte sits at the lowest offset. The 2.3 examples, including the 2-byte write that leaves its neighbor alone. Round trips at every width at the first and last valid offsets.
- [ ] T2.4 Accesses that cross the end throw, including a 4-byte read that starts 1, 2, or 3 bytes before the end, and the fault's `addr()`, `width()`, and `type()` are right. The last valid access at each width does not throw.
- [ ] T2.5 With two devices mapped, each address reaches the right device at the right offset (check through the `Memory` objects themselves). Unmapped addresses throw. An access that runs off the end of a device throws, even when another device starts right after it. Faults carry the bus address and the right `AccessType` (a failed `fetch` says `Fetch`). Overlapping ranges are rejected; touching ones are not.
- [ ] T2.6 A single typed test body covers the 8, 16, and 32-bit round trips through `read<T>` and `write<T>`.
- [ ] T2.7 The loader places bytes at the right addresses: after the 2.7 load, `fetch` returns 0x00500513. Loading past the end of memory throws.
- [ ] T2.8 The 2.8 example string, exactly. A 20-byte dump has two lines, the second with 4 bytes.

**Done when:** all tests pass and you can explain why instruction fetch and data access in your design reach the same memory.

**C++ focus:** abstract classes, pure virtual functions, virtual destructors, `override`, templates layered on virtual functions, writing your own concept, custom exceptions, `std::vector`, `std::span`, `std::unique_ptr` ownership, RAII.

### Lab 3: Register file and CPU state (S)

**Goal:** the 32 integer registers, the pc, and a way to compare and record CPU state.

**Where this fits:** decode reads source registers from the `RegisterFile`, and write back updates it. Tests compare whole `ArchState`s. Every instruction that retires produces a `CommitRecord`, which is the unit of truth for tracing (Lab 9), for comparing your two CPUs (Phase 5), and for checking your RTL core (Extension E6).

**Files:** add each `.cpp` to `rvcore` and each test file to `unit_tests`, as in Lab 2.

| File | What goes in it | Objectives |
|---|---|---|
| `include/rvsim/register_file.hpp`, `src/register_file.cpp` | `RegisterFile`, `abi_name` | 3.1, 3.2 |
| `include/rvsim/arch_state.hpp`, `src/arch_state.cpp` | `ArchState` and its `operator<<` | 3.3 |
| `include/rvsim/commit_record.hpp` | `RegWrite`, `MemWrite`, `CommitRecord` (header only) | 3.4 |
| `tests/unit/register_file_test.cpp` | T3.1, T3.2 | |
| `tests/unit/arch_state_test.cpp` | T3.3 | |
| `tests/unit/commit_record_test.cpp` | T3.4 | |

**Objectives**
- [ ] 3.1 **`RegisterFile`**: a class holding 32 registers of type `Word`, all 0 when constructed. Public members:

  | Member | Takes | Returns | Does |
  |---|---|---|---|
  | `read(index)` | `RegIndex` | `Word` | The value of that register; x0 always reads 0. `const`. |
  | `write(index, value)` | `RegIndex`, `Word` | nothing | Stores `value`. Writes to x0 are ignored. |
  | `operator==` | another `RegisterFile` | `bool` | Defaulted: equal when all 32 registers match. 3.3 uses it. |

  An index above 31 is a bug in your simulator, so `assert` it. Don't give the class an `operator[]` that returns a reference (`hints.md` 3.1 says why). Example: after writing 5 to x0 and 12 to x10, x0 reads 0 and x10 reads 12.
- [ ] 3.2 **ABI names**: a free function `abi_name(index)` that takes a `RegIndex` and returns its ABI name as a `std::string_view`. In order from x0 to x31: `zero ra sp gp tp t0 t1 t2 s0 s1 a0 a1 a2 a3 a4 a5 a6 a7 s2 s3 s4 s5 s6 s7 s8 s9 s10 s11 t3 t4 t5 t6`. Examples: 10 gives `a0`, 2 gives `sp`, and 8 gives `s0` (x8 is also called `fp`, but objdump prints `s0`). The 3.3 printout and Lab 5's disassembler use it.
- [ ] 3.3 **`ArchState`**: a struct with two members, `Addr pc` and `RegisterFile regs`, both starting at 0. Memory is not part of it: memory lives on the bus, and tests check it there.
  - A defaulted `operator==`: two states are equal only if the pc and every register match.
  - `operator<<(std::ostream&, const ArchState&)` prints the pc, then all 32 registers, four per line, each with its number, ABI name, and value as 8 hex digits. Spacing is up to you. The first lines for a state with pc 0x80000008, sp 0x800FFFF0, and a2 = 12:

  ```
  pc  80000008
  x0  zero 00000000   x1  ra   00000000   x2  sp   800ffff0   x3  gp   00000000
  x4  tp   00000000   x5  t0   00000000   x6  t1   00000000   x7  t2   00000000
  x8  s0   00000000   x9  s1   00000000   x10 a0   00000000   x11 a1   00000000
  x12 a2   0000000c   x13 a3   00000000   x14 a4   00000000   x15 a5   00000000
  ```
- [ ] 3.4 **`CommitRecord`**: one retired instruction, as plain values. Three structs, each with a defaulted `operator==`, because Phase 5 compares records:
  - `RegWrite`: `RegIndex rd` and `Word value`.
  - `MemWrite`: `Addr addr`, `std::size_t width`, and `Word value`. For a store, `value` holds only the bytes written, so an SB of 0x1234 records 0x34.
  - `CommitRecord`: `Addr pc`, `Word word` (the raw instruction), `std::optional<RegWrite> reg_write`, and `std::optional<MemWrite> mem_write`. Most instructions fill one of the two optionals, or neither.

  Examples:
  - `sub a3, a1, a0` at 0x8000000C with a0 = 5 and a1 = 7: pc 0x8000000C, word 0x40A586B3, `reg_write` of x13 = 2, no `mem_write`.
  - `sw a2, -4(sp)` at 0x80000010 with sp = 0x800FFFF0 and a2 = 12: pc 0x80000010, word 0xFEC12E23, no `reg_write`, and a `mem_write` of 12, 4 bytes wide, to 0x800FFFEC.

**Tests**
- [ ] T3.1 A new register file reads 0 everywhere. Writes to x0 are ignored; x1 through x31 round-trip.
- [ ] T3.2 ABI names: x0 `zero`, x1 `ra`, x2 `sp`, x8 `s0`, x10 `a0`, x31 `t6`.
- [ ] T3.3 States that differ only in the pc, or only in one register, compare unequal; identical states compare equal.
- [ ] T3.4 Two `CommitRecord`s that differ only in the width of their memory write compare unequal, and so do two that differ only in whether they have a register write.

**Done when:** tests pass and a state dump prints all 32 registers as a readable table.

**C++ focus:** `std::array`, const correctness, defaulted `operator==`, `operator<<`, `std::string_view`, `std::optional`.

---

## Phase 2: Decode

### Lab 4: Instruction fields and immediates (M)

**Goal:** pull every field out of a raw instruction and build all five immediate types correctly.

**Where this fits:** these are the decoder's tools. Lab 5's `decode()` calls them on every fetched word, and the instruction objects keep the results. Every function here takes the whole 32-bit instruction as a `Word`. Register fields come back as `RegIndex`; everything else comes back as a `Word`.

**Objectives**
- [ ] 4.1 Read the RV32I chapter and the instruction listing table in the unprivileged spec. Draw the six formats (R, I, S, B, U, J) by hand.
- [ ] 4.2 **`enum class Opcode`** with 11 enumerators (LUI, AUIPC, JAL, JALR, BRANCH, LOAD, STORE, OP-IMM, OP, MISC-MEM, SYSTEM), each set to its 7-bit opcode value from the spec.
- [ ] 4.3 **Field extractors** `opcode`, `rd`, `rs1`, `rs2`, `funct3`, `funct7`. Example: for 0x00B50633 (`add a2, a0, a1`) they return 0x33, 12, 10, 11, 0, and 0. For 0x40A586B3 (`sub a3, a1, a0`), `funct7` returns 0x20.
- [ ] 4.4 **Immediate builders** `imm_i`, `imm_s`, `imm_b`, `imm_u`, `imm_j`. Each returns the immediate already sign-extended to 32 bits. `imm_u` returns the value in its final position, with the low 12 bits zero. `imm_b` and `imm_j` return the byte offset, so bit 0 is always 0. See the examples below.
- [ ] 4.5 **Test vectors**: one assembly file, `tests/programs/lab4/vectors.S`, containing every RV32I instruction with chosen immediates (zero, positive, negative, and both range limits). Assemble it, run `objdump -d -M no-aliases`, and turn the output into a table of word, mnemonic, fields, and immediate in your test file.

**Immediate examples** (all from the cross assembler)

| Instruction | Word | Builder | Returns |
|---|---|---|---|
| `addi a0, zero, -1` | 0xFFF00513 | `imm_i` | 0xFFFFFFFF (-1) |
| `sw a2, -4(sp)` | 0xFEC12E23 | `imm_s` | 0xFFFFFFFC (-4) |
| `beq a0, a1, +8` | 0x00B50463 | `imm_b` | 8 |
| `bne a0, zero, -8` | 0xFE051CE3 | `imm_b` | 0xFFFFFFF8 (-8) |
| `lui a4, 0x12345` | 0x12345737 | `imm_u` | 0x12345000 |
| `jal ra, +16` | 0x010000EF | `imm_j` | 16 |
| `jal zero, -16` | 0xFF1FF06F | `imm_j` | 0xFFFFFFF0 (-16) |

**Tests**
- [ ] T4.1 Every immediate type: zero, small positive, small negative, largest positive, most negative. Start with the table above.
- [ ] T4.2 B and J immediates always have bit 0 clear; include backward branches and jumps.
- [ ] T4.3 Every vector from 4.5 yields the fields and immediates the disassembly shows.

**Done when:** every vector decodes to the right fields.

**C++ focus:** `enum class` and underlying types, `constexpr`, test fixtures, table-driven tests.

### Lab 5: Instruction class hierarchy and decoder (L)

**Goal:** turn a raw instruction into an object that knows what it is and what it does in each stage.

**Where this fits:** the decode stage hands a fetched `Word` and its pc to `decode()` and gets back an object. From then on the CPU never looks at bits again: it asks the object which registers it reads and writes, and (from Lab 6) tells it to execute.

**Objectives**
- [ ] 5.1 **Abstract `Instruction`**. Built from a pc (`Addr`) and a raw `Word`; it stores both, plus the decoded register numbers and immediate. It declares:
  - stage behavior: `execute()`, `access_memory(Bus&)`, and `write_back(RegisterFile&)`, whose bodies come in Lab 6
  - `disassemble()`, returning a `std::string`
  - queries that return `bool`: `reads_rs1()`, `reads_rs2()`, `writes_rd()`, `is_load()`, `is_control_flow()`, `is_system()`

  Examples: `add a2, a0, a1` reads rs1 and rs2 and writes rd. `sw a2, -4(sp)` reads rs1 and rs2 and writes nothing. `lui a4, 0x12345` reads nothing and writes rd.
- [ ] 5.2 Derived classes by category: `LoadInstruction`, `StoreInstruction`, `BranchInstruction`, `RegisterOpInstruction` (R-type), `ImmediateOpInstruction` (I-type arithmetic and shifts), `LuiInstruction`, `AuipcInstruction`, `JalInstruction`, `JalrInstruction`, `SystemInstruction` (ECALL, EBREAK), `FenceInstruction`, and `IllegalInstruction`.
- [ ] 5.3 Use class templates for the load family (width and signedness), the store family (width), and the branch family (comparison and signedness).
- [ ] 5.4 **`decode(word, pc)`**. Takes any `Word` and its `Addr`, and returns a `std::unique_ptr<Instruction>`. It never returns null and never throws: anything that isn't a valid RV32I instruction becomes an `IllegalInstruction`. Examples: `decode(0x00B50633, 0x80000008)` gives a `RegisterOpInstruction`, `decode(0x0FF0000F, pc)` gives a `FenceInstruction`, and `decode(0x00000000, pc)` gives an `IllegalInstruction`.
- [ ] 5.5 **Disassembly** in the same form as `objdump -d -M no-aliases`: the mnemonic, then operands separated by commas, with ABI register names, loads and stores written as `offset(base)`, and branch and jump targets as absolute addresses. Examples: `add a2,a0,a1`, `addi a0,zero,-1`, `sw a2,-4(sp)`, `lui a4,0x12345`, `beq a0,a1,8000001c` (the beq at 0x80000014), `jal ra,80000024` (a jal at 0x80000014), and `ecall`.
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

**Where this fits:** the CPU's execute, memory, and write back stages (Lab 7) are just calls to these methods, and the pipeline (Phase 5) calls the same ones. For one instruction the order is: the CPU gives it its operand values, then calls `execute()`, `access_memory()`, and `write_back()`. Afterwards `result()` holds the value for rd and `next_pc()` holds the address of the next instruction. (This is option A from hint 6.1; adjust the names if you pick option B.)

**Objectives**
- [ ] 6.1 Decide where operand values and results live (inside the instruction object, or in separate stage structs) and record it in `DESIGN.md`.
- [ ] 6.2 **ALU**: `alu(op, a, b)` takes an `AluOp` (ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND) and two `Word`s, and returns a `Word`. The I-type instructions reuse it, with the immediate as `b`. See the examples below.
- [ ] 6.3 **Arithmetic, logic, shifts, and comparisons**: `execute()` computes `result()`. Examples: `addi a0, zero, -1` gives 0xFFFFFFFF. `sltiu a0, a1, -1` with a1 = 5 gives 1, because the immediate becomes 0xFFFFFFFF and the comparison is unsigned.
- [ ] 6.4 **Control flow and upper immediates**: `next_pc()` is pc + 4 unless the instruction transfers control. Examples:
  - `beq a0, a1, +8` at 0x80000014: 0x80000018 when a0 and a1 differ, 0x8000001C when they're equal.
  - `jal ra, +16` at 0x80000014: `result()` is 0x80000018 (the link) and `next_pc()` is 0x80000024.
  - `jalr zero, 0(ra)` with ra = 0x80000101: `next_pc()` is 0x80000100 (bit 0 cleared).
  - `auipc a0, 0x1` at 0x80000000 gives 0x80001000; `lui a4, 0x12345` gives 0x12345000.
- [ ] 6.5 **Loads and stores**: `access_memory(bus)` does the access, and loads put the extended value in `result()`. Examples: if the byte at address A is 0x80, LB gives 0xFFFFFF80 and LBU gives 0x00000080. If the halfword there is 0x8000, LH gives 0xFFFF8000 and LHU gives 0x00008000. SH of 0x12345678 to A writes 78 56 to A and A + 1 and leaves A + 2 alone.
- [ ] 6.6 **Write back**: `write_back(regs)` writes `result()` to rd when `writes_rd()` is true. Nothing ever changes x0.
- [ ] 6.7 **Misaligned targets**: a taken transfer to an address that isn't a multiple of 4 is recorded as a fault on the instruction. Example: `jalr zero, 2(ra)` with ra = 0x80000000 targets 0x80000002 and faults. A not-taken branch never faults.

**ALU examples**

| Op | a | b | Result |
|---|---|---|---|
| ADD | 0x7FFFFFFF | 1 | 0x80000000 |
| SUB | 0 | 1 | 0xFFFFFFFF |
| SLT | 0xFFFFFFFF | 1 | 1 (signed: -1 < 1) |
| SLTU | 0xFFFFFFFF | 1 | 0 (unsigned: 0xFFFFFFFF > 1) |
| SRA | 0x80000000 | 4 | 0xF8000000 |
| SRL | 0x80000000 | 4 | 0x08000000 |
| SLL | 1 | 33 | 2 (only the low 5 bits of b count) |

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

**Where this fits:** this is the first time all the pieces run together, and at the end your Lab 0 program runs on your own CPU. This CPU is also the reference that the pipeline is checked against in Phase 5.

**Objectives**
- [ ] 7.1 **Stage structs**: IF/ID holds a valid bit, the pc, and the fetched `Word`. ID/EX, EX/MEM, and MEM/WB hold a valid bit and the instruction object (under option A it carries its own values). They become your pipeline registers in Phase 5.
- [ ] 7.2 **`SingleCycleCpu`**: its constructor takes a `Bus&` (the CPU uses the bus but doesn't own it). It owns the `RegisterFile` and the pc. `reset(entry, sp)` sets the pc to `entry`, sp to `sp`, and every other register to 0.
- [ ] 7.3 One method per stage: fetch, decode, execute, memory access, write back. Each takes the incoming stage struct and returns the outgoing one.
- [ ] 7.4 **`step()`** runs one instruction through all five stages. If the instruction retires, `step()` returns its `CommitRecord`. If it stops the CPU instead (an exit ECALL, EBREAK, a fault, or an illegal instruction), no record comes back, and `stop_reason()` says why. Following the spec, ECALL and EBREAK don't count as retired.
- [ ] 7.5 **`run(limit)`** calls `step()` until the CPU stops or `limit` instructions have retired, and returns the `StopReason`: exited (with its code), illegal instruction, memory fault, misaligned target, breakpoint (EBREAK), unknown system call, or instruction limit. Each reason carries the pc and raw word of the instruction involved. Example: `first.S` stops with "exited, code 12" after 9 retired instructions (its ECALL is the tenth and doesn't retire).
- [ ] 7.6 Faults are precise: a faulting instruction changes no register or memory, and the pc still points at it.
- [ ] 7.7 **Exit**: ECALL with a7 = 93 stops with "exited" and the code in a0. Any other a7 value stops with "unknown system call" and that value.
- [ ] 7.8 **Reset values**: with 1 MiB of memory at 0x80000000, reset with entry 0x80000000 and sp 0x800FFFF0 (16 bytes below the top, 16-byte aligned).
- [ ] 7.9 **Counters**: retired instructions and host time, so `run()` can report instructions per second.
- [ ] 7.10 Optional: a multi-cycle variant that advances one stage per call, so every instruction takes five cycles. It gives you a CPI baseline of 5 for Phase 5.

**Tests** (small hand-assembled programs written into memory by the test)
- [ ] T7.1 Sum 1 to 10 into a0 and exit: exit code 55.
- [ ] T7.2 Store and load round trips at every width.
- [ ] T7.3 A call and return with JAL and JALR, with a stack push and pop.
- [ ] T7.4 Forward and backward branches.
- [ ] T7.5 An illegal instruction and a fetch outside memory stop with the right reason and pc, and leave state unchanged.
- [ ] T7.6 The instruction limit stops an infinite loop.
- [ ] T7.7 The Lab 0 program (`tests/programs/lab0/first.S`) exits with code 12 after 9 retired instructions.

**Done when:** all tests pass.

**C++ focus:** composition, references for non-owning links, `std::variant` and `std::visit` (or `std::optional`) for stop reasons, `std::chrono`, exception boundaries.

### Lab 8: Running real programs (L)

**Goal:** compile C and assembly with the cross toolchain and run the results.

**Where this fits:** until now, test programs were words pasted into unit tests. From here on the simulator runs files built by the cross compiler, and every later lab uses this program suite.

**Files** (all new, under `tests/programs/`):

```
tests/programs/
├── Makefile
├── link.ld
├── runtime/        crt0.S, runtime.c, runtime.h
├── lab0/           first.S
└── suite/          one .c file per test program
```

**Objectives**
- [ ] 8.1 **Startup** (`runtime/crt0.S`): `_start` in section `.text.init` sets up the stack if needed, calls `main`, then makes the exit ECALL (a7 = 93), so main's return value becomes the exit code.
- [ ] 8.2 **Linker script** (`link.ld`): one RAM region at 0x80000000, 1 MiB long, with `.text.init` first. Check: `readelf -l` on any program shows its first LOAD segment starting at 0x80000000, as in Step 0.9. The link address must equal the load address.
- [ ] 8.3 **Build** (`Makefile`): `make` builds every program in `suite/` at `-O0` and at `-O2`, into `build/<name>-O0.elf` and `build/<name>-O2.elf`, with a `.dump` (from `objdump -d -M no-aliases`) next to each for debugging. Flags: `-march=rv32i -mabi=ilp32`, freestanding, no standard library. The tool prefix lives in one variable, for example `CROSS ?= riscv64-unknown-elf-`.
- [ ] 8.4 **Runtime** (`runtime/runtime.c`): the pieces the compiler calls on its own: `memset`, `memcpy`, `memmove`, and `memcmp`, plus multiply and divide helpers unless you link `-lgcc`.
- [ ] 8.5 **Flat binaries** first: `load_binary(bus, path, base)` copies a file made with `objcopy -O binary` to `base` and returns `base` as the entry point.
- [ ] 8.6 **ELF loader**: `load_elf(bus, path)` validates the header (magic, 32-bit, little-endian, RISC-V), copies each loadable segment, zero-fills the rest of each segment's memory size, and returns the entry point. Invalid files throw a `LoadError` with a readable message. Example: `first.elf` gives entry 0x80000000 and one 0x28-byte segment copied to 0x80000000.
- [ ] 8.7 **Console**: `ConsoleDevice` takes a `std::ostream&` and is mapped at 0x10000000. A 1-byte write of 0x41 prints `A`. Add `print_char`, `print_str`, and `print_hex` to your C runtime.
- [ ] 8.8 **Command line**: `rvsim <program.elf> [--max-instructions N] [--trace] [--model single]`. The process exit code equals the guest's exit code. Any other stop prints the reason to stderr and exits with a code your programs never return, such as 255.
- [ ] 8.9 **Program suite** in `suite/`, each self-checking: `main` returns 0 on success and a distinct nonzero code for each failed check. Programs: Fibonacci (loop and recursive), bubble sort, string functions, GCD, shift-and-add multiply, xorshift random numbers, Collatz, and a "hello" that prints.
- [ ] 8.10 Build and run every program at `-O0` and `-O2`.

**Tests**
- [ ] T8.1 ELF loader unit tests on a small ELF checked into the repository (`tests/data/`): entry point, segment bytes, zeroed bss.
- [ ] T8.2 The loader rejects bad magic, 64-bit files, big-endian files, the wrong machine, and segments outside memory.
- [ ] T8.3 One CTest entry per program per optimization level, passing when the simulator exits with 0.

**Done when:** every program passes at both optimization levels and "hello" prints.

**C++ focus:** binary file I/O, `std::filesystem`, parsing binary formats field by field, exceptions with context, command-line parsing, process exit codes.

### Lab 9: Tracing and the official tests (M)

**Goal:** see exactly what the CPU did, and prove it against the official ISA tests.

**Where this fits:** from here on, every bug hunt starts with a trace or a commit-log diff, and the official tests guard every later change.

**Objectives**
- [ ] 9.1 **Trace**: one line per retired instruction with a count, pc, word, disassembly, and what it wrote. Example: `     3  80000008  00b50633  add a2,a0,a1        x12 <= 0x0000000c`.
- [ ] 9.2 `--trace` prints the trace to the terminal and `--trace-file <path>` writes it to a file. Off by default, and nearly free when off.
- [ ] 9.3 **Commit log** (`--commit-log <path>`): only pc, word, and writes, one line per retired instruction, so two runs can be compared with `diff`. Examples: `80000008 00b50633 x12=0000000c` for a register write, and `80000010 fec12e23 mem[800fffec]=0000000c/4` for a 4-byte store.
- [ ] 9.4 **riscv-tests (rv32ui)** built with your own minimal environment in `tests/riscv-tests-env/` (your `riscv_test.h` and `link.ld`), so they run without CSRs or traps. Each test exits with 0 on pass, or with the number of the failing case.
- [ ] 9.5 Register every applicable rv32ui test with CTest, named `rv32ui-add`, `rv32ui-beq`, and so on.
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

**Where this fits:** the pipeline reuses the bus, register file, decoder, and instruction objects unchanged; only the control around them is new. That is why the single-cycle CPU can serve as its reference. `first.S` has data hazards (its `add` reads a0 and a1 right after they're written) and a branch, so it runs on the pipeline after Lab 12; make a NOP-padded copy for this lab.

**Objectives**
- [ ] 10.1 **`CpuModel`** interface with `reset(entry, sp)`, `step()`, `run(limit)` returning the `StopReason`, `state()` returning the `ArchState`, `stats()`, and `set_commit_callback()`. `step()` means one instruction on the single-cycle CPU and one clock cycle on the pipeline; `run()` works the same on both. Both CPUs implement it, so the simulator (`--model single` or `--model pipeline`) and the tests can use either.
- [ ] 10.2 **`PipelinedCpu`** with the four stage structs from 7.1 as its pipeline registers. A register whose valid bit is off holds a bubble.
- [ ] 10.3 **`tick()`** advances one clock cycle: compute every stage's output from the current register contents, then update all registers together.
- [ ] 10.4 Commit happens only in WB. Exits, faults, and illegal instructions take effect only when they commit.
- [ ] 10.5 Reuse the Lab 6 instruction behavior. The pipeline only moves instructions between stages and calls their methods.
- [ ] 10.6 **Pipeline diagram** (`--pipeview`): one row per cycle with the pc each stage holds, or `.` for a bubble. The first five cycles of a program at 0x80000000:

```
cycle  IF        ID        EX        MEM       WB
    1  80000000  .         .         .         .
    2  80000004  80000000  .         .         .
    3  80000008  80000004  80000000  .         .
    4  8000000c  80000008  80000004  80000000  .
    5  80000010  8000000c  80000008  80000004  80000000
```

- [ ] 10.7 **Stats**: cycles, retired instructions, and CPI (cycles divided by retired instructions).
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

**Where this fits:** hazard handling comes down to two small decisions made every cycle: where each operand comes from, and whether to stall. Write each as a pure function that takes plain values and returns a decision; the pipeline just calls them.

**Objectives**
- [ ] 11.1 Write back updates the register file before decode reads it in the same cycle.
- [ ] 11.2 **Forwarding**: a function that takes the register an EX instruction reads, plus what EX/MEM and MEM/WB hold (valid, writes rd, rd, is load), and returns where the value comes from: the register file, EX/MEM, or MEM/WB. Newest value first, never for x0. Examples: EX reads x5 while both EX/MEM and MEM/WB write x5: take EX/MEM. EX reads x0 while EX/MEM writes x0: take the register file.
- [ ] 11.3 Forwarded values reach every consumer: ALU operands, branch comparisons, the JALR base, and store data.
- [ ] 11.4 **Load-use stall**: a function that takes what ID/EX holds and the instruction in ID, and returns whether to stall. On a stall, hold the pc and IF/ID for one cycle and send a bubble into ID/EX. Examples: `lw a0, 0(sp)` in ID/EX with `add a1, a0, a0` in ID: stall. With `lui a0, 1` in ID instead: no stall, because LUI reads no registers.
- [ ] 11.5 A register counts as used only if the instruction actually reads it.
- [ ] 11.6 Stats: stalls, and forwards by source.

**Tests** (branch-free programs)
- [ ] T11.1 A register read 1, 2, and 3 instructions after it is written.
- [ ] T11.2 Two consecutive writes to the same register, then a read: the newer value wins.
- [ ] T11.3 A write to x0 followed by a read of x0 reads 0.
- [ ] T11.4 A load followed immediately by a use: correct value and exactly one extra cycle, so a program of N instructions with one such pair takes N + 5 cycles.
- [ ] T11.5 A load followed by a use two instructions later: correct value and no stall.
- [ ] T11.6 A store whose base and data registers were just written, and a load whose base was just written.
- [ ] T11.7 Unit tests for the forwarding and stall decision functions on their own.
- [ ] T11.8 Every program above also runs on the single-cycle model with an identical commit log, and cycle counts equal N + 4 + stalls.

**Done when:** every branch-free hazard program matches the single-cycle model, including the expected cycle counts.

**C++ focus:** small pure functions that are easy to test, a shared test helper that runs one program on both models.

### Lab 12: Control hazards (M)

**Goal:** branches and jumps work in the pipeline.

**Where this fits:** with control hazards handled, every program runs on the pipeline. From here on the differential check (both CPUs, identical commit logs) guards every change.

**Objectives**
- [ ] 12.1 **Resolve in EX**: each instruction compares its `next_pc()` with the pc that was fetched after it (pc + 4, since you predict not taken). A mismatch produces a redirect to `next_pc()`.
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
- [ ] T12.8 `first.S` exits with code 12 in 14 cycles: 10 instructions plus 4, with no stalls (all its hazards are covered by forwarding) and no redirect (its branch is not taken).

**Done when:** T12.7 passes. Tag v0.2.

**C++ focus:** invariants checked with assertions, `std::optional` for redirect requests.

---

## Phase 6: Branch prediction

### Lab 13: Predictor framework (M)

**Goal:** predict in IF, check in EX, recover on a mispredict.

**Where this fits:** IF now guesses the next pc instead of always using pc + 4, and EX checks the guess with the Lab 12 redirect machinery. Predictors are interchangeable objects chosen at startup.

**Objectives**
- [ ] 13.1 **`BranchPredictor`** interface: `predict(pc)` returns `true` for taken, `update(pc, taken)` trains on the real outcome, and `name()` returns a label for the stats.
- [ ] 13.2 **Target prediction** in IF: a direct-mapped `Btb` whose `lookup(pc)` returns an optional target and whose `update(pc, target)` records one, or pre-decoding direct branches in IF. Record which one you chose.
- [ ] 13.3 Carry each instruction's predicted next pc through the pipeline with it.
- [ ] 13.4 In EX, a mispredict is any difference between the actual next pc and the predicted next pc.
- [ ] 13.5 Recovery reuses the Lab 12 redirect path.
- [ ] 13.6 Static predictors: always not taken, always taken, and backward taken, forward not taken (BTFN).
- [ ] 13.7 **Registry**: `make_predictor(name)` returns a `std::unique_ptr<BranchPredictor>` for `nt`, `t`, and `btfn` (and `bimodal` after Lab 14), selected with `--predictor <name>`. An unknown name lists the valid ones.
- [ ] 13.8 **Stats**: conditional branches, mispredictions, accuracy (correct predictions divided by conditional branches), MPKI (mispredictions per 1000 retired instructions), and BTB hit rate.

**Example:** a loop branch that is taken 9 times and then not taken once. Always-not-taken mispredicts 9 times. Always-taken with a BTB mispredicts twice: once on the first iteration, while the BTB is still empty, and once at the exit.

**Tests**
- [ ] T13.1 Always-not-taken reproduces the Lab 12 cycle counts exactly.
- [ ] T13.2 For a loop with a known iteration count, each static predictor mispredicts exactly the number of times you computed on paper.
- [ ] T13.3 The differential check still passes with every predictor: prediction never changes architectural results.

**Done when:** T13.3 passes for every predictor.

**C++ focus:** the strategy pattern, a factory registry, `std::function`, dependency injection.

### Lab 14: Bimodal predictor and results (M)

**Goal:** the classic table of 2-bit counters, measured on your programs.

**Where this fits:** bimodal is the first predictor that learns. It plugs into the Lab 13 framework as one more name in the registry.

**Objectives**
- [ ] 14.1 **`SaturatingCounter`** class template with the bit width as a template parameter. It holds a value from 0 to 2^Bits - 1; `increment()` and `decrement()` stop at the ends; `predict_taken()` is true in the upper half of the range. Example with 2 bits: values 0 to 3, taken at 2 and 3. From 1, one taken outcome moves it to 2 and flips the prediction.
- [ ] 14.2 **`BimodalPredictor`**: its constructor takes the table size (a power of two) and the initial counter value. The index is the pc shifted right by 2, masked to the table size. Example with 16 entries: pc 0x80000040 uses entry 0, 0x80000044 uses entry 1, and 0x80000080 uses entry 0 again (the two alias).
- [ ] 14.3 Make the counters' initial state a command-line option.
- [ ] 14.4 Train on conditional branches only, in EX.
- [ ] 14.5 **Sweep** table sizes (for example 4 to 4096 entries) and predictors over the program suite, writing one CSV row per run with the columns program, predictor, entries, instructions, cycles, branches, mispredictions, accuracy, mpki, and cpi.
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

**Where this fits:** a `MemoryHierarchy` object slots in between the CPU and the bus. In this lab it only counts hits and misses; Lab 16 makes misses cost cycles.

**Objectives**
- [ ] 15.1 **`Cache`**: built from a name, a total size, a block size, and a number of ways (all powers of two). `access(addr, is_write)` returns whether it hit and updates the cache state. Example: 4 KiB with 16-byte blocks, direct-mapped, has 256 sets, so an address splits into a 4-bit offset, an 8-bit index, and a 20-bit tag. Address 0x80001234 splits into tag 0x80001, index 0x23, and offset 0x4.
- [ ] 15.2 A replacement policy interface with LRU, plus FIFO or random for comparison.
- [ ] 15.3 Hit, miss, and eviction stats for reads and writes.
- [ ] 15.4 **`MemoryHierarchy`**: the same fetch, read, and write calls as the `Bus`. Fetches go through an L1 instruction cache, loads and stores through an L1 data cache, misses in either go to a shared L2, and the data itself always comes from the bus. The CPU now talks to this instead of to the bus.
- [ ] 15.5 Caches track metadata only. Data always comes from memory, so a cache bug can never corrupt results. Device addresses (the console) bypass the caches.
- [ ] 15.6 **Options**: `--l1i`, `--l1d`, and `--l2`, each written as `size:block:ways` (for example `--l1d 4096:16:2`). Every run prints its effective configuration.

**Tests**
- [ ] T15.1 Tag, index, and offset for several geometries, starting with the example in 15.1.
- [ ] T15.2 In a direct-mapped cache, two alternating addresses that share an index always miss.
- [ ] T15.3 A 2-way cache removes that conflict.
- [ ] T15.4 A known access sequence evicts the blocks LRU says it should.
- [ ] T15.5 The differential check passes.

**Done when:** miss rates for the program suite are recorded for several geometries.

**C++ focus:** composition, policy classes as templates and as virtual interfaces (try both), `std::list` with `std::unordered_map`, the decorator pattern.

### Lab 16: Cache timing and results (M)

**Goal:** cache misses cost cycles in the pipeline.

**Where this fits:** the memory hierarchy now reports how long each access took, and the pipeline stage that made the access waits that long.

**Objectives**
- [ ] 16.1 **Latency**: every access returns the extra cycles it cost: 0 on an L1 hit, the L2 latency when L1 misses and L2 hits, and the L2 latency plus the memory latency when both miss. Example with an L2 latency of 10 and a memory latency of 100: 0, 10, or 110.
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
