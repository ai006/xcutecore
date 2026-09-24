# Hints

Hints are keyed to the objective numbers in `labs.md`. Try the objective first. **Gotcha** marks bugs that commonly cost hours. There is no code here. Names are suggestions: types in PascalCase and functions in snake_case. If you prefer snake_case types like `load_instruction`, rename as you go and stay consistent. Labs with their own walkthrough file (`hints_lab2.md` so far) are covered there instead; this file keeps the general notes and the hints for the other labs.

## Notes on the fmash16 post

It's good for the overall shape: its DRAM, BUS, and CPU structs map to your `Memory`, `Bus`, and CPU classes, and it uses the same 0x80000000 memory base as QEMU and Spike. Keep these differences and problems in mind:

- It models RV64 (64-bit registers). You are building RV32I.
- Its U-type immediate mask is a typo that keeps some of the low 12 bits. The text also calls rd, rs1, and rs2 4-bit fields; they are 5 bits.
- Its SLTI compares an unsigned register with a signed immediate, which C and C++ quietly turn into an unsigned comparison.
- Its SRAI shifts by the whole I-type immediate, which still contains the bit that marks SRAI, so the shift amount is far too large (undefined behavior).
- It links programs at address 0 but loads them at 0x80000000 (see 8.2).
- It increments pc before executing, zeroes x0 every cycle, and stops when pc becomes 0. All three work for its simple loop and break in a pipeline (see 3.1, 6.4, 7.5).
- It sets sp to exactly the end of memory (see 7.8).

## General

- Run the tests after every objective, not every lab.
- Keep all logic in the core library. The simulator's `main` should only parse options, build objects, and run.
- Store register values as unsigned 32-bit integers everywhere. Convert to signed only at the exact spot an operation needs signed meaning: SLT, SLTI, SRA, SRAI, signed branches, and sign extension. This one rule prevents most CPU model bugs.

## Lab 0

Lab 0 is a step-by-step walkthrough with code in `labs.md`, so it has no hints. If a step fails, start with that step's troubleshooting notes.

## Lab 1

- **1.1** `std::uint32_t` for `Word` and `Addr`, and `std::int32_t` for `SWord`, from `<cstdint>`. For `RegIndex`, `std::uint8_t` is compact but needs a `static_cast` whenever you store a `Word` in it (the conversion warnings will insist); plain `unsigned` avoids the casts. Aliases don't stop you from mixing `Word` and `Addr`, since the compiler sees the same type, but they make every signature say what a number means. See the signed and unsigned rule under General.
- **1.2** Two steps: shift the value right by `lo` so the field starts at bit 0, then clear everything above the field with a mask of `hi - lo + 1` ones.
- **1.2** **Gotcha:** building that mask by shifting 1 left by the field width is undefined when the width is 32. Handle the full-width case separately, or build the mask in a 64-bit type.
- **1.3** `bit` is a one-bit `bits`: extract the field from `n` to `n` and compare it with 1.
- **1.4** First keep only the low `width` bits (1.2 does that), then extend. Two common ways to extend. One: shift left so the field's sign bit lands in bit 31, convert to signed, then shift right arithmetically by the same amount. Two: flip the field's sign bit with xor, then subtract that bit's value. In C++20 both are fully defined for 32-bit integers.
- **1.5** A `constexpr` helper can be checked with `static_assert`, which fails the build instead of a test run. `assert` is allowed inside `constexpr` functions: at run time it works as usual, and a failing `assert` during compile-time evaluation becomes a compile error. Keep a few `static_assert`s as smoke checks and put the big tables in runtime tests.
- **1.6** Build the string from bit 31 down to bit 0 with your own `bit()`, adding a space after every fourth bit except the last. `reserve(39)` (32 digits plus 7 spaces) avoids reallocations. `std::bitset<32>(value).to_string()` gives the digits without spaces if you want a shortcut.
- **T1.2** One parameterized suite per helper (`TEST_P` with `INSTANTIATE_TEST_SUITE_P`) turns each row of the worked-examples table into one line.
- **T1.3** `EXPECT_DEATH(statement, "")` passes when the statement kills the process. Asserts disappear when `NDEBUG` is defined, and release builds define it, so build the death test only when `NDEBUG` is not defined (or skip it with `GTEST_SKIP()`).
- **Gotcha:** integer promotion. A `uint8_t` or `uint16_t` inside an expression becomes a signed `int` before shifts and arithmetic, so shifting a byte left by 24 lands in the sign bit of an `int`. Convert to your 32-bit unsigned type before shifting.

## Lab 2

Lab 2 has its own walkthrough: `hints_lab2.md`.

## Lab 3

- **3.1** Avoid a non-const `operator[]` that returns a reference: `regs[0] = 5` would compile and break x0. Use explicit read and write functions, and ignore writes to index 0. That is cleaner than re-zeroing x0 after every instruction (as the fmash16 post does), because a pipeline has no single "after every instruction" moment.
- **3.1** An index outside 0 to 31 is a bug in your simulator, not in the guest program, so `assert` is the right tool.
- **3.2** A `static constexpr std::array` of 32 `std::string_view`s. x8 is both `s0` and `fp`; objdump prints `s0`.
- **3.3** In C++20 you can default `operator==` for a class whose members are comparable, private members included. `std::array` already compares element by element, so `RegisterFile`'s defaulted `==` works, and `ArchState`'s works because `RegisterFile`'s does.
- **3.4** Use `std::optional` for the register write and for the memory write, since most instructions do one or neither. Record the width of memory writes. Keep the record small and made of plain integers: you create one per instruction, and in Extension E6 it crosses into SystemVerilog through DPI-C. A defaulted `operator==` works here too, since `std::optional` of a comparable type is comparable.
- **T3.3, T3.4** **Gotcha:** with `-Wextra`, GCC 13 rejects a designated initializer that leaves out a member, such as `CommitRecord{.pc = 0x80000010, .word = 0xFEC12E23, .mem_write = w}`, with `missing initializer for member` (`-Wmissing-field-initializers`). Name every member, writing `.reg_write = std::nullopt` for an empty optional, or set the fields one at a time after construction.

## Lab 4

- **4.1** The spec's figure of the base formats and its "RV32/64G Instruction Set Listings" table give every opcode, funct3, and funct7 you need. Drawing the formats yourself is the fastest way to remember where the immediate bits sit.
- **4.2** Every 32-bit RISC-V instruction ends in binary 11. RV32I uses 11 major opcodes: LUI, AUIPC, JAL, JALR, BRANCH, LOAD, STORE, OP-IMM, OP, MISC-MEM, and SYSTEM. Take the values from the spec table.
- **4.3** rd, rs1, and rs2 are 5 bits wide; funct3 is 3; funct7 and the opcode are 7. Extract all of them for every instruction, even fields a format doesn't use. Deciding which fields matter is the decoder's job.
- **4.4** I-type is one contiguous field, sign-extended. S-type is the same 12 bits split into two pieces around the rd position. U-type keeps the upper 20 bits where they are with the low 12 bits zero, so no extension step is needed on RV32.
- **4.4** **Gotcha:** B and J are where most decoders go wrong. Their immediate bits are shuffled so the sign bit is always instruction bit 31 and the other bits line up with the S and U formats. Bit 0 of the immediate is always 0 and isn't stored. Build each piece from the spec figure one at a time, and test backward offsets and both range limits.
- **4.5** Place labels a known distance away (or write targets relative to `.`, the current address) so you control the exact offsets; objdump then shows the resulting target address. Another trick: encode a word by hand with the `.word` directive and check that objdump disassembles it to what you intended.
- **4.5** **Gotcha:** objdump prints many instructions under alias names (`li`, `mv`, `j`, `ret`, `nop`, `beqz`). Add `-M no-aliases` to see the real instruction, and `-M numeric` if you want x-register numbers instead of ABI names.

## Lab 5

- **5.1** Suggested behavior methods: `execute()`, `access_memory(Bus&)`, `write_back(RegisterFile&)`, and `disassemble()`. Make `execute` and `disassemble` pure virtual. Give `access_memory` a base version that does nothing, and `write_back` a base version that writes the result to rd when the instruction writes rd, so most derived classes override only one or two methods.
- **5.1** Suggested queries: `reads_rs1()`, `reads_rs2()`, `writes_rd()`, `is_load()`, `is_control_flow()`, `is_system()`. Your Phase 5 hazard logic is built entirely on these, so get them right now and test them (T5.4).
- **5.1** Suggested value slots, used from Lab 6 on: a setter for the rs1 and rs2 operand values, `result()` for the value headed to rd, and `next_pc()`. In the pipeline, forwarding overwrites the operands before `execute` runs.
- **5.1** The base class needs a virtual destructor, and leaf classes can be marked `final`. You will move `std::unique_ptr<Instruction>` around rather than copy instructions, so consider deleting the copy operations in the base.
- **5.2** Two ways to cover the ten R-type operations with one class: store an operation enum and switch on it, or store a callable. Start with the enum; it's easy to print and test. `ImmediateOpInstruction` can share the same enum and ALU.
- **5.2** ECALL and EBREAK share every field except the immediate (0 for ECALL, 1 for EBREAK). Any other SYSTEM encoding is a CSR or privileged instruction and is illegal for now.
- **5.2** FENCE is MISC-MEM with funct3 000; on a single-core model it does nothing. FENCE.I (funct3 001) belongs to the Zifencei extension, so treat it as illegal until Extension E5.
- **5.3** `LoadInstruction` as a class template on the loaded type: `std::int8_t` for LB, `std::uint8_t` for LBU, `std::int16_t` for LH, `std::uint16_t` for LHU, and a 32-bit type for LW. `sizeof(T)` gives the width, and converting the loaded bytes to `T` and then to your register type gives sign or zero extension for free. `std::is_signed_v<T>` and `sizeof(T)` can even generate the mnemonic.
- **5.3** `StoreInstruction` on `std::uint8_t`, `std::uint16_t`, and `std::uint32_t`. Converting the value to `T` does the truncation.
- **5.3** `BranchInstruction` on a comparison function object and an operand type: `std::equal_to<>` and `std::not_equal_to<>` for BEQ and BNE, `std::less<>` and `std::greater_equal<>` with `std::int32_t` for BLT and BGE, and the same two with `std::uint32_t` for BLTU and BGEU. Six instructions from one class.
- **5.3** A class template can derive from the non-template `Instruction`. Each instantiation is its own derived class, and the decoder still returns all of them as `std::unique_ptr<Instruction>`.
- **5.3** **Gotcha:** template member functions must be visible where the template is used, so define them in the header (or explicitly instantiate the few types you use in a source file). Otherwise you get linker errors that look unrelated.
- **5.4** Switch on the opcode, then funct3, then funct7 where needed. Every path that doesn't match a real instruction returns an `IllegalInstruction` instead of throwing. The reason shows up in Lab 12: a pipeline decodes garbage on the wrong path all the time, and garbage may only become an error if it commits.
- **5.4** Encodings that are easy to accept by mistake: JALR needs funct3 000. R-type needs funct7 0000000, except SUB and SRA, which use 0100000. SLLI needs the upper seven immediate bits to be 0000000; SRLI and SRAI need 0000000 or 0100000. BRANCH funct3 010 and 011 are unused. LOAD funct3 011 and 110 and STORE funct3 011 are RV64-only. The all-zeros and all-ones words are illegal by definition.
- **5.5** Match objdump's `-M no-aliases` output so you can diff against it. Loads and stores print as `offset(base)`, and objdump prints branch and jump targets as absolute addresses.
- **T5.1** Store the Lab 4 vectors as a table of (word, expected mnemonic, expected class) and run one parameterized test over it. `dynamic_cast`, or a virtual kind query, lets the test check the class.

## Lab 6

- **6.1** Option A: the instruction object carries its operand values and results, so it travels through the pipeline with everything it needs. Option B: instruction objects are read-only descriptions and all values live in the stage structs, which is closer to hardware. Option A fits the hierarchy you built; choose it unless you have a reason not to.
- **6.2** An `AluOp` enum class and a free function taking the op and two operands. With `-Wswitch-enum`, the compiler tells you when a case is missing.
- **6.3** **Gotcha:** signed overflow is undefined behavior in C++. Do ADD and SUB on unsigned 32-bit values, where wraparound is defined.
- **6.3** **Gotcha:** shifting by 32 or more is undefined. RV32 uses only the low 5 bits of the shift amount, so mask first. The fmash16 SRAI bug is this one.
- **6.3** SRA: convert to signed, shift, convert back. C++20 guarantees an arithmetic shift for negative values and defines those conversions, which is another reason to require C++20.
- **6.3** SLTIU sign-extends the immediate first and then compares unsigned, so an immediate of -1 compares against 0xFFFFFFFF.
- **6.3** **Gotcha:** comparing a signed value with an unsigned one converts the signed side to unsigned (the fmash16 SLTI bug). `-Wsign-compare`, which `-Wall` enables for C++, flags it. Convert both sides explicitly.
- **6.4** Branch and jump targets are relative to the instruction's own pc, not pc + 4. Keep the pc inside the instruction object. Incrementing pc before executing (as the fmash16 loop does) forces a correction later and falls apart in a pipeline, where the CPU's pc belongs to a different instruction.
- **6.4** JALR's target is the rs1 value plus the immediate, with bit 0 cleared. Because the rs1 value was captured at decode, rd equal to rs1 needs no special care in your model. It does in a naive implementation that writes rd first.
- **6.4** The link value for JAL and JALR is the instruction's pc + 4. AUIPC produces pc plus the U immediate; LUI produces the U immediate.
- **6.5** Effective address: the rs1 value plus the sign-extended immediate, wrapping. Stores write the low 8, 16, or 32 bits of the rs2 value.
- **6.6** The base `write_back` checks `writes_rd()`. The register file also ignores x0, so a write to x0 is harmless either way.
- **6.7** B and J offsets are always even but not always multiples of 4, and JALR clears only bit 0, so all three can produce a target that isn't a multiple of 4. The spec raises the exception on the branch or jump itself, and only when the transfer is taken. Record it as a fault on the instruction and let the CPU decide what to do with it.
- **Tests** A helper in the test file that builds an instruction, sets operands, runs it, and returns what you want to check keeps each parameterized row (`TEST_P` with `INSTANTIATE_TEST_SUITE_P`) to one line. Good edge values for each operand: 0, 1, -1, the most negative, and the most positive. Good shift amounts: 0, 1, 31, and 32 in rs2 (which must act like 0).

## Lab 7

- **7.1** Roughly what each struct needs: IF/ID holds the pc and raw bits (later also a fetch fault and prediction data); ID/EX, EX/MEM, and MEM/WB hold the instruction object, which under option A carries its own values. Add a valid bit to each now.
- **7.2** Composition: the CPU has a `RegisterFile` and a pc, and keeps a reference to a `Bus` passed to its constructor. Tests can then build their own bus with whatever devices they need.
- **7.3** Make each stage a method that takes the incoming struct and returns the outgoing struct. `step()` then reads like the textbook, and Phase 5 becomes an extension instead of a rewrite.
- **7.4** Update the pc at the end of `step()` from `next_pc()`. Build the `CommitRecord` in write back.
- **7.5** A `std::variant` of small structs (exited with a code, illegal instruction, memory fault, misaligned target, breakpoint, limit reached) plus `std::visit` for printing is a good use of both. An enum with extra fields also works.
- **7.6** Catch the bus exception at the stage boundary and turn it into a stop reason before write back runs. Loads and stores fault in the memory stage, before any state has changed, as long as the pc update comes last.
- **7.7** 93 is the Linux RISC-V exit system call number and a common convention for bare-metal simulators. Read a0 and a7 from the register file in write back.
- **7.8** **Gotcha:** if sp equals the memory base plus the size (as in the fmash16 post), any read at 0(sp) is out of bounds. Start sp a little below the end, aligned to 16 (the calling convention keeps the stack 16-byte aligned). If you ever use newlib's startup code, it reads argc from 0(sp), so that word must exist and be 0.
- **7.9** `std::chrono::steady_clock` around `run()`. Don't optimize anything yet; Extension E8 is where speed work belongs.
- **7.10** An `enum class` for the current stage and a switch in `step()`. The stage structs become member variables so they persist between calls.
- **Tests** Assemble test programs with the cross assembler and paste the words into the test as an array, one word per line with its instruction in a comment. Tests that need unusual layouts (like T12.3 later) can use a tiny memory, such as 64 bytes.

## Lab 8

- **8.1** The startup file needs: a global `_start` in its own section (for example `.text.init`, which the linker script places first), stack pointer setup (or rely on the simulator's reset value), a call to `main`, then ECALL with a7 set to the exit number (main's return value is already in a0). Put an infinite loop after the ECALL in case it ever returns.
- **8.2** The linker script needs `ENTRY(_start)`, a `MEMORY` region at your base with your size, and output sections for `.text` (with `.text.init` first), `.rodata`, `.srodata`, `.data`, `.sdata`, `.bss`, and `.sbss`. Export a symbol at the end of bss if you want a heap later.
- **8.2** **Gotcha:** the fmash16 post links at address 0 (`-Ttext=0x0`) but loads the binary at 0x80000000. Any code that uses an absolute address, such as a pointer to a global variable, then points at nothing. The link address must equal the load address.
- **8.2** **Gotcha:** if your linker script defines `__global_pointer$`, the linker may rewrite global variable accesses to be relative to gp, and then your startup code must set gp (with linker relaxation turned off for that one instruction). If you don't define the symbol, you can ignore gp.
- **8.3** Flags to start with: `-march=rv32i -mabi=ilp32 -ffreestanding -nostdlib -static -T <your script> -Wall`, plus the optimization level. Add `-fno-builtin` if your own string functions clash with GCC's built-in versions. Keep the flags in one place.
- **8.4** **Gotcha:** RV32I has no multiply or divide instructions, so C code with `*`, `/`, or `%` calls libgcc helpers (`__mulsi3`, `__divsi3`, `__udivsi3`, `__modsi3`, `__umodsi3`). With `-nostdlib` you must add `-lgcc` yourself, after your object files, or write the helpers. A shift-and-add `__mulsi3` is a good warm-up.
- **8.4** **Gotcha:** GCC can emit calls to `memcpy`, `memset`, `memmove`, and `memcmp` even in freestanding code (struct copies, large zeroed arrays). Provide them.
- **8.5** objcopy's binary output spans everything from the lowest to the highest loadable address, so sections far apart make a huge file; one region avoids that. A flat binary has no entry point, so `_start` must be the first byte and execution starts at the base.
- **8.6** `man 5 elf` describes the layout. Header fields you need: the identification bytes (magic 0x7F 'E' 'L' 'F', 32-bit class, little-endian data), the machine (EM_RISCV, 243), the entry point, and the location, entry size, and count of the program headers. From each program header: the type (PT_LOAD), file offset, physical or virtual address (the same in your setup), file size, and memory size. Copy the file bytes, then zero up to the memory size; that zero tail is bss.
- **8.6** **Gotcha:** don't read the file into a buffer and `reinterpret_cast` it to a header struct. Padding, alignment, and strict aliasing make that fragile. Read each field at its documented offset with a little-endian helper, or `std::memcpy` it into a variable of the right type.
- **8.6** Validate before trusting anything: offsets and sizes inside the file, segments inside memory, and file size no larger than memory size.
- **8.7** A `ConsoleDevice` derived from `Device`, mapped outside RAM (0x10000000 is where QEMU's virt machine puts its UART). Writes append a character to an output stream injected through the constructor: `std::cout` in the app, a `std::ostringstream` in tests. Reads can return 0.
- **8.8** Parsing `argv` by hand is fine for a handful of options. Print usage for unknown options.
- **8.9** xorshift32 produces pseudo-random numbers with only shifts and xor, so it needs no multiply. Return a different nonzero code from each check so a failing CTest names the check.
- **T8.1** Build the small ELF once with the cross toolchain and commit it under `tests/data`, so unit tests don't need the toolchain installed.
- **T8.3** `add_test` with the simulator as the command. CTest fails a test when the exit code is nonzero.

## Lab 9

- **9.1** Example contents of one trace line: count, pc, raw word, disassembly, then something like `x10 <= 0x00000037` or `mem[0x80001000] <= 0x12 (1 byte)`.
- **9.2** Either pass a pointer to a stream that may be null, or register a commit callback (a `std::function` taking a `CommitRecord`) that the app sets. Check once per instruction, and never build strings when tracing is off.
- **9.3** Keep the commit log to pc, raw word, register write, and memory write. Two models that commit the same instructions then produce byte-identical logs.
- **9.4** Clone riscv-tests with its submodules (the `env` folder is one). The stock `p` environment uses CSRs and machine-mode traps, which you don't have yet, so make your own environment: copy `env/p/riscv_test.h` and `env/p/link.ld` into a new folder, keep the macro names the tests use (`RVTEST_RV32U`, `RVTEST_RV64U`, `RVTEST_CODE_BEGIN`, `RVTEST_CODE_END`, `RVTEST_PASS`, `RVTEST_FAIL`, `RVTEST_DATA_BEGIN`, `RVTEST_DATA_END`; check the sources for any others), and rewrite their bodies. Code begin only needs to start `_start` in `.text.init`. Pass sets a0 to 0 and makes your exit ECALL. Fail puts the failing test number in a0 and makes the same ECALL; the test macros keep that number in gp and call it `TESTNUM`. The data macros can be nearly empty. Point the linker script at your memory base.
- **9.4** Most rv32ui sources redefine the RV64 setup macro as the RV32 one and then include the matching rv64ui file. Build each with `-march=rv32i -mabi=ilp32`, include paths for your environment folder and `isa/macros/scalar`, and your linker script.
- **9.4** **Gotcha:** newer GCC and binutils moved the CSR instructions and FENCE.I out of the base `i` extension (into Zicsr and Zifencei). An "unrecognized opcode" error on a `csr` instruction means CSR code is still in your environment; on `fence.i` it means you are building a test that needs Zifencei.
- **9.5** Skip `fence_i` (it needs Zifencei). If anything else fails, assume it's your bug: the test number in the exit code tells you which case failed, and objdump of the test plus your trace shows where.
- **9.6** Spike can print a commit log (see its `-l` and `--log-commits` options), but it needs its own `tohost` environment to finish a program, so this is much easier after Extension E5.
- **T9.2** Compare the trace against a checked-in file, and add a way to regenerate that file (a flag or an environment variable) for intentional format changes.

## Lab 10

- **10.1** An abstract `CpuModel` with virtual `run`, `step` (one instruction for the single-cycle model, one clock cycle for the pipeline), `state`, `stats`, and a commit callback setter. A test that takes a `CpuModel&` runs unchanged on both models.
- **10.2** A pipeline register: a valid bit, a `std::unique_ptr<Instruction>`, the pc, and later prediction and fault data. A default-constructed register is a bubble.
- **10.3** Two ways to avoid ordering bugs. (a) Keep `current` and `next` pipeline state, compute every stage from `current` into `next`, then replace `current` with `next` at the end of the cycle. (b) Evaluate the stages in reverse order (WB, MEM, EX, ID, IF) so each stage consumes its input register before the stage in front of it overwrites that register. (a) mirrors hardware and makes stalls and flushes easier to reason about; (b) is shorter. Record your choice.
- **10.3** C++ helps here: a `std::unique_ptr` can't be copied, so one instruction can't accidentally sit in two stages, and moving it forward leaves an empty pointer (a bubble) behind. **Gotcha:** a stalled stage must move its instruction into the same register of the next state, or the instruction silently disappears.
- **10.4** Anything visible outside the CPU (register writes, memory writes, exits, error reports) happens only for valid instructions that are allowed to commit. Stores write memory in MEM, which is safe only because nothing in MEM can still be flushed while branches resolve in EX. Write that invariant down; Lab 16 tests it, and Extension E9 breaks it on purpose.
- **10.5** If you notice yourself copying execute logic into the pipeline, stop. The pipeline moves objects and calls their methods.
- **10.6** Fixed-width columns: the cycle number, then the pc or mnemonic in each stage, with a dot for a bubble. Print only when a flag is on.
- **10.8** (Von Neumann) In one cycle, IF reads an instruction while MEM may read or write data, and both live in the same memory. A single-port memory can't serve both at once, which is why textbook pipelines draw separate instruction and data memories. Your choices: treat memory as having two ports (one address space, two accesses per cycle; what most models do), or model one port and stall IF whenever MEM uses memory. Lab 15's split instruction and data caches are how real machines resolve this.
- **10.8** Another Von Neumann consequence: a store can overwrite an instruction that has already been fetched and is sitting in IF or ID, and the pipeline will execute the stale copy. RISC-V's answer is FENCE.I, which flushes everything behind it. Note it for Extension E5.
- **10.9** A read must come at least 3 instructions after the write (2 NOPs between them) if write back happens before decode in the same cycle (11.1), or at least 4 after (3 NOPs) if not. End every program with the exit ECALL.
- **T10.1** The first instruction commits in cycle 5 and the last in cycle N + 4. Decide whether your cycle counter starts at 0 or 1, and test that too.

## Lab 11

- **11.1** The register file is architectural state, not pipeline state. With current and next state, let WB write it before ID reads it within the same cycle; with reverse-order evaluation this happens on its own. It reproduces the textbook rule of writing in the first half of the cycle and reading in the second half.
- **11.2** Forward from EX/MEM when its instruction is valid, writes rd, has an rd other than 0, and that rd matches a register the EX instruction reads. Otherwise check MEM/WB with the same conditions. EX/MEM wins because it holds the newer value.
- **11.2** Under option A, the forwarded value is `result()` of the instruction in that stage. A load in EX/MEM has no value yet; the load-use stall in 11.4 guarantees you never need one, so assert that forwarding never picks a load from EX/MEM.
- **11.2** Write hazard detection and forwarding selection as small free functions over plain values (valid, writes rd, rd, the register being read) that return a decision. They are easy to unit test without building a pipeline (T11.7).
- **11.3** Overwrite the instruction's operand values with the forwarded ones before `execute()` runs. Stores use rs1 as the base and rs2 as the data; JALR uses rs1; branches compare both.
- **11.4** The situation: the load is in EX (held in ID/EX) and the reader is in ID (held in IF/ID). To stall, keep the pc and IF/ID unchanged, put a bubble into ID/EX for the next cycle, and let the load continue into MEM. One cycle later the reader moves to EX and receives the loaded value forwarded from MEM/WB.
- **11.5** LUI, AUIPC, and JAL read no registers, and only R-type instructions, stores, and branches read rs2. Treating unused fields as reads doesn't change results (the value is ignored), but it adds stalls real hardware wouldn't have, and your cycle-count tests will catch them.
- **Tests** A helper that runs a program on both models and compares final state and commit logs will be the most reused function in your test suite. Write expected cycle counts as the formula (N + 4 + stalls) so each test explains itself.

## Lab 12

- **12.1** In EX, compare the instruction's actual next pc with the pc that was fetched after it (pc + 4 for now). A mismatch means redirect. Framing it this way turns Lab 13 into a small change.
- **12.2** When the branch in EX redirects, the instruction being decoded and the one being fetched in that same cycle are on the wrong path. The next IF/ID and ID/EX become bubbles, the next pc becomes the correct target, and the branch itself moves on to MEM.
- **12.3** Suggested priority, highest first: a redirect from EX, then a stall, then the normal pc + 4. A flush of IF/ID overrides a stall of IF/ID, since the stalled instruction is on the wrong path anyway. In this 5-stage design a load-use stall and a redirect can't happen in the same cycle (the EX instruction can't be both a load and a branch), but memory stalls in Lab 16 break that assumption, so write the rule down now.
- **12.4** Give IF/ID a fault field. When a fetch throws, catch it in IF and record the fault instead of raw bits; decode turns it into an instruction object that stops the simulation only if it commits. Illegal instructions already work this way from Lab 5. Treat load and store faults in MEM the same way: record them on the instruction and act at commit. Nothing should throw past a stage.
- **12.4** Those wrong-path instructions are your model's transient window. They can't change architectural state, but they can change microarchitectural state: from Lab 15 on, wrong-path fetches touch the instruction cache. Extension E9 builds on this.
- **12.5** **Gotcha:** if ECALL captured a0 and a7 at decode, it could see stale values: the instructions just before it may not have written back yet, and ECALL's rs fields are zero, so forwarding won't fix it. Read them from the register file at commit.
- **12.7** When a program fails only on the pipeline, diff the two commit logs. The first differing line tells you which cycles to inspect in the pipeline diagram.
- **T12.2** Put the bad instruction in the fall-through position right after a taken branch, not at the target.
- **T12.3** Use a tiny memory and write the words directly: jump to the last word, which holds a backward jump to the exit code. The two fetches after that jump fall outside memory, get recorded as faults, and are flushed.

## Lab 13

- **13.1** `predict(pc)` returns taken or not taken, and `update(pc, taken)` trains. Add `name()` for the stats output, and a virtual destructor.
- **13.2** BTB: index with pc bits (drop the low 2 bits, which are always 0), and store a valid bit, a tag (the remaining upper bits), and the target. The next pc is the stored target only when the BTB hits and the direction predictor says taken; otherwise it's pc + 4. Update the entry whenever a control-flow instruction is taken.
- **13.2** The pre-decode alternative: IF decodes just enough of the fetched word to compute pc plus the immediate for branches and JAL. It's simpler, can't predict JALR, and hides the BTB's cold-start cost. Fine for a first pass.
- **13.3** Store the predicted next pc in IF/ID and hand it to the instruction object (or ID/EX) at decode.
- **13.4** One comparison, actual next pc against predicted next pc, covers a wrong direction, a wrong target (a stale BTB entry, or JALR), and a BTB hit on an instruction that isn't a taken control transfer (partial tags or self-modifying code).
- **13.5** Recovery redirects to the actual next pc, which is pc + 4 when a branch predicted taken turned out not taken.
- **13.6** BTFN needs the target to tell whether a branch goes backward, so it depends on the BTB or pre-decode.
- **13.7** A `std::unordered_map` from name strings to `std::function`s that return a `std::unique_ptr<BranchPredictor>`. Print the valid names when the user gives an unknown one.
- **13.8** Train in EX. Every instruction in EX is on the correct path (wrong-path instructions were flushed when the older branch resolved), so predictors never learn from wrong-path branches. MPKI is mispredictions per 1000 committed instructions.
- **T13.2** The branch that closes an N-iteration loop is taken N - 1 times and then not taken once. Count each predictor's mispredictions on paper first, including the BTB's first miss, then assert those numbers.

## Lab 14

- **14.1** `SaturatingCounter` needs increment, decrement, a taken prediction (the upper half of the range), and the raw value. Store the count in the smallest unsigned type that fits, and `static_assert` that the width is between 1 and 8.
- **14.2** Index: the pc shifted right by 2, masked to the table size. `std::has_single_bit` checks for a power of two and `std::countr_zero` gives log2. The size is a runtime value, so reject bad sizes by throwing from the constructor.
- **14.3** Weakly not taken is the textbook default; weakly taken often does better on loop-heavy code. Measure both.
- **14.4** JAL and JALR are always taken, so only the BTB learns them. The counters train on conditional branches only.
- **14.5** A small script (Python or shell) that runs every program with every predictor and size and writes one CSV row per run. Have the simulator print its stats in a machine-readable form (key=value lines or JSON) so the script stays trivial.
- **14.6** Plot MPKI against table size for each program. Your programs have few branches, so aliasing only shows at tiny sizes. A larger workload such as CoreMark (it needs a small port) makes the curves more interesting.
- **T14.3** With a 4-entry table, pick two branch addresses that differ only in bits above the index.
- **T14.5** Walk a 2-bit counter through T, N, T, N by hand from each starting state to see why "at least half" is the right assertion.

## Lab 15

- **15.1** Offset bits are log2 of the block size, index bits are log2 of the number of sets, and the tag is the rest. Each line stores a valid bit, a tag, and whatever the replacement policy needs. No data.
- **15.2** A possible policy interface: `touch(set, way)` and `victim(set)`. LRU can be a per-line timestamp of last use, or a per-set list of ways in use order. Try the policy once as a template parameter and once as a virtual interface, and compare the code you end up with.
- **15.4** A `MemoryHierarchy` object between the CPU and the bus, with fetch and data entry points: fetches go through L1I, data goes through L1D, and misses in either go to L2. The data itself always comes from the bus. The separate fetch entry point from 2.5 pays off here.
- **15.5** Because the caches hold no data, a cache bug shows up as wrong statistics, never as wrong results, so the differential check stays meaningful. Writes still count as accesses and update LRU order. Check the address against the RAM range before involving the caches, so console writes skip them.
- **15.6** A config struct with defaults, filled from command-line options and printed at the start of every run, so result files describe themselves.
- **T15.2** In a direct-mapped cache, two addresses with the same index and different tags miss on every access (ping-pong).

## Lab 16

- **16.1** An L1 miss costs the L1 latency plus the L2 latency, plus the memory latency if L2 also misses. Store latencies in cycles.
- **16.2** Model the wait as a countdown in the stage. A waiting stage holds its instruction and makes everything behind it hold too: a MEM wait freezes MEM, EX, ID, and IF, while WB finishes its instruction and then receives a bubble.
- **16.2** **Gotcha:** a MEM wait and a taken branch in EX in the same cycle. The branch can't leave EX while MEM is frozen, so flushing ID/EX that cycle deletes the branch itself. Either apply the redirect only in the cycle the branch actually advances, or flush only the instructions behind it, keep the branch, and mark it resolved so it doesn't redirect again. This is the case 12.3 said couldn't happen before caches.
- **16.3** In hardware, wrong-path fetches do touch the cache, and a wrong-path miss may still be in progress when the flush arrives. Decide whether a flush cancels a pending wait or lets it finish, and record the choice. It matters for Extension E9.
- **16.4** Count stall cycles by cause so CPI splits into base, branch penalty, load-use, and memory stalls.
- **T16.1** Run the same program with latency 0 and with latency L around a single known miss.

## Extensions

- **E1** gshare: a global history register of the last N branch outcomes, xored with pc bits to form the index. Updating history at EX is the simple start; updating it speculatively in IF and repairing it on a mispredict is the realistic version.
- **E2** The unprivileged spec's JALR section has a table of return-address stack hints: x1 and x5 count as link registers, and the combination of rd and rs1 tells you whether to push, pop, or both.
- **E4** Division by zero returns all ones as the quotient and the dividend as the remainder. Dividing the most negative value by -1 returns the dividend with remainder 0. **Gotcha:** that same division is undefined behavior in C++, so check for it before dividing. The MULH variants need a 64-bit intermediate; take extra care with MULHSU's mixed signs.
- **E5** You need the CSR instructions, the CSRs the environment's startup code touches, trap entry through mtvec, and MRET. Read `RVTEST_CODE_BEGIN` in the stock environment: it probes some optional CSRs by letting the write trap, so unimplemented CSRs must raise an illegal instruction exception. Find `tohost` in the ELF symbol table; a write there ends the test. After that the unmodified `rv32ui-p-*` tests run, and comparing with Spike (9.6) becomes easy.
- **E6** Expose a small C-linkage API (`extern "C"`): create a model, load a program, step one instruction, and return the last `CommitRecord` as plain integers. Verilator calls it through DPI-C, and the testbench compares every instruction the RTL retires with the model's record. The commit log format from 9.3 is already the right shape.
- **E7** Konata (github.com/shioyadan/Konata) reads its own Kanata log format and gem5's O3PipeView format. Emit an event when an instruction enters each stage, commits, or gets flushed.
- **E8** Profile with `perf`. The likely costs are one heap allocation and several virtual calls per instruction. Ideas: an object pool, or a decode cache keyed by pc. **Gotcha:** the same pc can occupy two stages at once in a tight loop, so a cached object can't hold per-execution values. Split the immutable decoded part from the per-execution state, or cache prototypes and give `Instruction` a virtual `clone()` that returns a `std::unique_ptr<Instruction>`. Invalidate entries when a store writes to a cached address (Von Neumann again).
- **E9** In a 5-stage pipeline that resolves branches in EX, wrong-path instructions never reach MEM, so only the instruction cache sees them and a data cache leak can't happen. Add an option to resolve branches later (or deepen the pipeline) so wrong-path loads reach the data cache. That breaks the 10.4 invariant, so first move store writes to commit (or buffer them until commit). Then train a bounds check with in-range indices, call it once with an out-of-range index, and show that the data cache state afterward depends on the out-of-range byte.
- **E10** Make the register value type a template parameter or a build-time option, then add RV64I's W instructions and 6-bit shift amounts.
- **E11** RISCOF needs a plugin for your model, and your model must write the memory between the `begin_signature` and `end_signature` symbols to a file at exit.

## When something breaks

1. Find the first wrong commit, using the trace or a diff of two commit logs.
2. Reproduce it in the smallest program you can, then turn that into a unit test.
3. Check the usual suspects: signed versus unsigned, shift masking, which pc was used, x0, and immediate bit order.
4. For pipeline bugs, print the pipeline diagram for the cycles leading up to the failure and walk it by hand.
5. Keep the sanitizers on. They turn silent corruption into a stack trace.
