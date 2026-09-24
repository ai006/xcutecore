# xCuteCore

A RISC-V RV32I CPU model in C++20, built step by step from a single-cycle CPU into a cycle-level five-stage pipeline with branch prediction and caches.

**Status:** early. Labs 0 and 1 are done, and Lab 2 (memory and the bus) is in progress. The CPU doesn't run RISC-V code yet; the first program runs in Lab 7.

**Why "xCuteCore":** **Xcute** is a play on execute, the heart of the fetch–decode–execute pipeline, and **Core** because it models a single in-order CPU core. Small, focused, and a little cute.

## About

xCuteCore models a small computer: an RV32I CPU and a RAM on a shared bus, with instructions and data in one address space (a Von Neumann machine). The CPU starts unpipelined and takes each instruction through all five stages before starting the next. Once it runs C programs and passes the official `rv32ui` riscv-tests, the next phase is changing to a cycle-level five-stage pipeline with forwarding and stalls. Branch prediction and a cache hierarchy come after that, each measured on a suite of test programs.

The project has one purposes:

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
│   └── DESIGN.md           design decisions and their reasons
├── include/rvsim/
│   ├── types.hpp           Word, Sword, Addr, regIndex
│   ├── bits.hpp            bits, bit, sign_extend, to_binary_string
│   ├── device.hpp          Device, the abstract base for anything on the bus
│   ├── memory.hpp          Memory
│   └── version.hpp
├── src/                    the rvcore library: memory.cpp, version.cpp
├── sim/main.cpp            the rvsim executable (prints its version for now)
└── tests/
    ├── unit/               GoogleTest: smoke_test, test_bits, test_memory
    └── programs/lab0/      first.S, the first hand-written RV32I program
```

## Progress

| Phase | Labs | Milestone | Status |
|---|---|---|---|
| 0. Setup | 0 | Build, unit tests, and cross toolchain work | Done, tagged `v0.0` |
| 1. Architectural state | 1, 2, 3 | Bit helpers, memory and bus, registers | In progress |
| 2. Decode | 4, 5 | Every RV32I instruction decodes and disassembles | Not started |
| 3. Execute | 6 | Every instruction's behavior is unit tested | Not started |
| 4. Unpipelined CPU | 7, 8, 9 | Runs C programs and the official rv32ui tests | Not started (tag `v0.1`) |
| 5. Pipeline | 10, 11, 12 | The pipeline commits exactly what the unpipelined CPU commits | Not started (tag `v0.2`) |
| 6. Branch prediction | 13, 14 | BTB and bimodal predictor, with measured results | Not started (tag `v0.3`) |
| 7. Caches | 15, 16 | Cache hierarchy with timing, with measured results | Not started (tag `v0.4`) |
| 8. Extensions | menu | Pick what interests you | Not started |

### Work done

**Lab 0: Project setup** (tag `v0.0`)
- CMake project with three targets: the `rvcore` static library, the `rvsim` simulator, and the `unit_tests` GoogleTest executable, registered with CTest.
- Warnings as errors through an interface target (`rvsim_options`), so they apply to project code only. The `RVSIM_SANITIZE` option adds ASan and UBSan, with `-fno-sanitize-recover=all` so undefined behavior fails the test.
- The harness was proven by breaking it on purpose: a failing test, an unused variable, a heap overflow, and a signed overflow each failed the build or the test run.
- clang-format (Google style, 100 columns), and a `compile_commands.json` link for clangd.
- Cross toolchain checked: `riscv64-unknown-elf-gcc` 10.2.0 has an `rv32i/ilp32` multilib, and 32-bit libgcc links.
- `tests/programs/lab0/first.S`, the first RV32I program, assembled and inspected with `objdump` and `readelf` (ELF32, little-endian, RISC-V, entry `0x80000000`). It will be the first program the CPU runs, in Lab 7, and should exit with code 12.
- `docs/DESIGN.md` started with decisions D1 to D4.

**Lab 1: Bit toolkit**
- `types.hpp`: `Word` and `Addr` (unsigned 32-bit), `Sword` (signed 32-bit), and `regIndex` (a register number).
- `bits.hpp`: `bits(value, hi, lo)`, `bit(value, n)`, and `sign_extend(value, width)`, all `constexpr`, `noexcept`, and `[[nodiscard]]`, with preconditions checked by `assert`. Also `to_binary_string` for debugging.
- Tests: six `static_assert`s checked at compile time, and 15 runtime tests covering every row of the lab's worked-examples table plus the binary string.

**Lab 2: Memory and the bus** (in progress)
- [x] 2.1 `Device`: an abstract base with a virtual destructor and pure virtual `size()`, `read(offset, width)`, and `write(offset, width, value)`.
- [x] 2.2 `Memory`: a `Device` backed by a zero-filled `std::vector<std::uint8_t>`, with 1-byte reads and writes.
- [x] T2.1 `static_assert`s that `Device` is abstract and has a virtual destructor.
- [x] T2.2 A new memory reads zero everywhere, a byte round-trips, and a write of `0x1FF` stores `0xFF` without touching the next byte.

As of 2026-09-24, the sanitizer build (g++-13) compiles with zero warnings and all 17 tests pass.

### What's left

**Finish Lab 2**
- [ ] 2.3 Little-endian 2- and 4-byte access in `Memory`, aligned or not
- [ ] 2.4 `AccessType` and `MemoryFault`, with bounds checks that throw
- [ ] 2.5 `Bus`: `map`, `fetch`, `read`, and `write`; faults carry bus addresses, and overlapping mappings are rejected
- [ ] 2.6 Typed access: a `BusValue` concept with `read<T>` and `write<T>`
- [ ] 2.7 `load_bytes`, which places bytes on the bus
- [ ] 2.8 `hex_dump`
- [ ] Tests T2.3 to T2.8

**Loose ends in Lab 1**
- T1.2 asks for more than the worked-examples table: bits 0 and 31 of `0x80000001`, `bits` with `hi` equal to `lo`, and `sign_extend` at widths 1, 5, 8, 12, 13, 16, 20, 21, and 32, using the largest positive and the most negative value at each. The tests cover the table rows so far.
- T1.3, the optional death test, is checked off in `labs.md`, but `test_bits.cpp` has no death test.
- Objective 1.1 puts the type aliases in the `rvsim` namespace, but `types.hpp` declares them at global scope. `regIndex` also breaks D4's PascalCase rule for types; the lab calls it `RegIndex`.

**Remaining labs**

Sizes (S, M, L) are relative effort.

| Lab | What gets built | Done when |
|---|---|---|
| 3. Register file and CPU state (S) | `RegisterFile` with x0 fixed at 0, ABI names, `ArchState`, `CommitRecord` | A state dump prints all 32 registers as a readable table |
| 4. Instruction fields and immediates (M) | `Opcode` enum, field extractors, I/S/B/U/J immediate builders, test vectors checked against objdump | Every vector decodes to the right fields |
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
