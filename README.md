# xCuteCore

A RISC-V RV32I CPU model in C++20, built step by step from a single-cycle CPU into a cycle-level five-stage pipeline with branch prediction and caches.

**Status:** early. Labs 0 to 4 are done: the architectural state (Phase 1) is complete, and instructions can be cut into their fields and immediates. Lab 5 (instruction class hierarchy and decoder) is next. The CPU doesn't run RISC-V code yet; the first program runs in Lab 7.

**Why "xCuteCore":** **Xcute** is a play on execute, and **Core** because it models a single in-order CPU core. Small, focused, and a little cute.

## About

xCuteCore models a small computer: an RV32I CPU and a RAM on a shared bus, with instructions and data in one address space (a Von Neumann machine). The CPU starts unpipelined and takes each instruction through all five stages before starting the next. Once it runs C programs and passes the official `rv32ui` riscv-tests, the next phase is changing to a cycle-level five-stage pipeline with forwarding and stalls. Branch prediction and a cache hierarchy come after that, each measured on a suite of test programs.

The project has one purpose:

- **Learning.** It follows a 17-lab path, [`docs/labs.md`](docs/labs.md), that teaches computer architecture and modern C++ side by side. Every lab has objectives, tests, and a "done when" check, and each piece is tested before the next one starts.

### Design decisions

The reasons for each are in [`docs/DESIGN.md`](docs/DESIGN.md).

| Decision | Choice |
|---|---|
| ISA | RV32I, little-endian |
| Memory map | RAM at `0x80000000`, 1 MiB by default: the same base as Spike, QEMU `virt`, and riscv-tests |
| Halting | `ecall` with `a7 = 93` exits, with the exit code in `a0` |
| Naming | Types in PascalCase; functions and variables in snake_case |

## Build and test

You need GCC 13 or newer (`sim/main.cpp` includes `<format>`, which GCC 11's standard library lacks), CMake 3.22 or newer, and GoogleTest. The `riscv64-unknown-elf` cross toolchain builds the guest programs. Lab 0 in [`docs/labs.md`](docs/labs.md) walks through the full setup.

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DRVSIM_SANITIZE=ON -DCMAKE_CXX_COMPILER=g++-13
cmake --build build/debug -j
ctest --test-dir build/debug --output-on-failure
./build/debug/sim/rvsim        # prints "rvsim 0.0.1"
```

All project code builds with `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wswitch-enum -Werror`. `RVSIM_SANITIZE=ON` adds AddressSanitizer and UndefinedBehaviorSanitizer, and tests run in that build by default. For timing, use a release build in `build/release` with `-DCMAKE_BUILD_TYPE=Release` and no sanitizers.

## Repository layout

```
.
├── CMakeLists.txt          warning and sanitizer options, subdirectories
├── docs/
│   ├── labs.md             the lab path: objectives, tests, and "done when" checks
│   ├── hints.md            general notes, and hints for labs without their own file
│   ├── hints_lab2.md       Lab 2 walkthrough
│   ├── hints_lab4.md       Lab 4 walkthrough
│   └── DESIGN.md           design decisions and their reasons
├── include/rvsim/
│   ├── types.hpp           Word, Sword, Addr, RegIndex
│   ├── bits.hpp            bits, bit, sign_extend, to_binary_string
│   ├── device.hpp          Device, the abstract base for anything on the bus
│   ├── memory.hpp          Memory
│   ├── memory_fault.hpp    AccessType, MemoryFault
│   ├── bus.hpp             Bus, the BusValue concept, typed read<T> and write<T>
│   ├── loader.hpp          load_bytes
│   ├── hex_dump.hpp        hex_dump
│   ├── register_file.hpp   RegisterFile, abi_name
│   ├── arch_state.hpp      ArchState: the pc and the registers, printed as a table
│   ├── commit_record.hpp   RegWrite, MemWrite, CommitRecord
│   ├── fields.hpp          Opcode, field extractors, immediate builders (header only)
│   └── version.hpp
├── src/                    the rvcore library: memory, memory_fault, bus, loader, hex_dump,
│                           register_file, arch_state, version
├── sim/main.cpp            the rvsim executable (prints its version for now)
└── tests/
    ├── unit/               GoogleTest: smoke_test, test_bits, test_memory, test_bus, test_loader,
    │                       register_file_test, arch_state_test, commit_record_test, fields_test,
    │                       and rv32i_vectors.hpp (the Lab 4 test vectors as a table)
    └── programs/
        ├── lab0/           first.S, the first hand-written RV32I program
        └── lab4/           vectors.S (all 40 RV32I instructions) and its objdump listing
```

## Progress

| Phase | Labs | Milestone | Status |
|---|---|---|---|
| 0. Setup | 0 | Build, unit tests, and cross toolchain work | Done, tagged `v0.0` |
| 1. Architectural state | 1, 2, 3 | Bit helpers, memory and bus, registers | Done |
| 2. Decode | 4, 5 | Every RV32I instruction decodes and disassembles | In progress |
| 3. Execute | 6 | Every instruction's behavior is unit tested | Not started |
| 4. Unpipelined CPU | 7, 8, 9 | Runs C programs and the official rv32ui tests | Not started (tag `v0.1`) |
| 5. Pipeline | 10, 11, 12 | The pipeline commits exactly what the unpipelined CPU commits | Not started (tag `v0.2`) |
| 6. Branch prediction | 13, 14 | BTB and bimodal predictor, with measured results | Not started (tag `v0.3`) |
| 7. Caches | 15, 16 | Cache hierarchy with timing, with measured results | Not started (tag `v0.4`) |
| 8. Extensions | menu | Pick what interests you | Not started |

### Work done

- **Lab 0: Project setup** (tag `v0.0`): CMake build with the `rvcore` library, the `rvsim` simulator, and GoogleTest unit tests. Warnings are errors, sanitizers are on, and the cross toolchain assembles `first.S`.
- **Lab 1: Bit toolkit**: the `Word`, `Addr`, and register types, plus `bits`, `bit`, and `sign_extend` for pulling fields out of instructions.
- **Lab 2: Memory and the bus**: a little-endian `Memory` device, a `Bus` that maps devices and reports faults, typed `read<T>` and `write<T>`, `load_bytes`, and `hex_dump`.
- **Lab 3: Register file and CPU state**: a `RegisterFile` whose x0 always reads 0, ABI register names, an `ArchState` (the pc and the registers) that compares with `==` and prints all 32 registers as a table, and the `CommitRecord` that each retired instruction will produce.
- **Lab 4: Instruction fields and immediates**: an `Opcode` enum for the 11 RV32I opcodes, `constexpr` extractors for the opcode, register, and funct fields, and builders for the sign-extended I, S, B, U, and J immediates. They're checked against 66 test vectors that cover all 40 RV32I instructions, with every immediate at zero and both range limits, assembled by the cross toolchain and read back with objdump.

As of 2026-10-07, the sanitizer build (g++-13) compiles with zero warnings and all 177 tests pass.

### What's left

Lab 5 is next. Sizes (S, M, L) are relative effort.

| Lab | What gets built | Done when |
|---|---|---|
| 5. Instruction classes and decoder (L) | `Instruction` class hierarchy (with templates for loads, stores, and branches), a `decode()` that never throws, objdump-style disassembly | Every vector decodes to the right class and prints correctly |
| 6. Execution semantics (L) | ALU, plus execute, memory, and write back behavior for every instruction, including misaligned-target faults | Every RV32I instruction has at least one behavior test |
| 7. Single-cycle CPU (M) | `SingleCycleCpu` with `step()` and `run()`, stop reasons, precise faults, exit through `ecall` | `first.S` exits with code 12 after 9 retired instructions |
| 8. Running real programs (L) | Startup code, linker script, Makefile, C runtime, binary and ELF loaders, a console device at `0x10000000`, a command line, and a self-checking C program suite | Every program passes at `-O0` and `-O2`, and "hello" prints |
| 9. Tracing and the official tests (M) | Instruction trace, commit log, and the `rv32ui` riscv-tests in a minimal environment | All applicable rv32ui tests pass; tag `v0.1` |
| 10. Pipeline structure (L) | `CpuModel` interface, `PipelinedCpu` with a per-cycle `tick()`, pipeline diagram, CPI stats | Hazard-free programs match the single-cycle CPU |
| 11. Data hazards (M) | Forwarding and load-use stalls, as small pure decision functions | Branch-free programs match, with the expected cycle counts |
| 12. Control hazards (M) | Branches resolved in EX; wrong-path instructions flushed without ever becoming visible | Every program and rv32ui test gives identical commit logs on both CPUs; tag `v0.2` |
| 13. Predictor framework (M) | `BranchPredictor` interface, BTB, static predictors (not taken, taken, BTFN), a predictor registry, accuracy and MPKI stats | The differential check passes with every predictor |
| 14. Bimodal predictor and results (M) | `SaturatingCounter` template, `BimodalPredictor`, a sweep script and plot | Results table and plot in this README; tag `v0.3` |
| 15. Cache model (L) | `Cache` with LRU and a second replacement policy, and a `MemoryHierarchy` of L1I, L1D, and a shared L2 | Miss rates recorded for several cache geometries |
| 16. Cache timing and results (M) | Miss latencies that stall IF and MEM, stall cycles by cause, a CPI sweep | CPI results in this README; tag `v0.4` |

**Extensions (Phase 8, pick any):** gshare (E1), a return address stack (E2), a tournament predictor (E3), RV32M multiply and divide (E4), Zicsr and machine-mode traps to run the unmodified riscv-tests (E5), a reference model for the SystemVerilog core through Verilator DPI-C (E6), Konata pipeline visualization (E7), simulator speed with a decode cache (E8), a transient-execution playground (E9), RV64I (E10), and the RISC-V architecture tests through RISCOF (E11).
