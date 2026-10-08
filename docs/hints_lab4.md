# Lab 4 hints: Instruction fields and immediates

A walkthrough for Lab 4 in `labs.md`. Part numbers match objective numbers (Part 4 is objective 4.4). Each part explains the idea, the C++ it needs, gives a scaffold with TODOs, lists the gotchas, and says how to check your work. Scaffolds show where things go, not the code: you write every declaration and every body. Try each objective before reading its part.

---

## Overview

In plain words: an instruction is a 32-bit number, and this lab teaches your code to read it. Think of a paper form with boxes at fixed places. Once you know the date box is in the top right corner, you can read the date without reading anything else. An RV32I instruction is one of six forms (the formats), and every box sits at fixed bit positions. The extractors (4.3) read one box each. The immediate, a constant built into the instruction, is the one awkward box: in some formats it's split into pieces scattered around the form, and the builders (4.4) put the pieces back together in the right order.

```
Bus::fetch(pc) returns the Word 0x00B50633                       Lab 2, called from Lab 7
  │
  ├── opcode, funct3, funct7 ──► which instruction: ADD           4.2, 4.3
  ├── rd, rs1, rs2           ──► which registers: x12, x10, x11   4.3
  └── imm_i ... imm_j        ──► the constant (ADD has none)      4.4
  ▼
decode(word, pc) builds an instruction object from those pieces   Lab 5
```

All the cutting is done by your Lab 1 helpers. Almost every function in this lab is a few calls to `bits` and, for immediates, one call to `sign_extend`. The hard part isn't the C++. It's getting every bit to the right place, which is why 4.1 is a drawing and 4.5 makes the cross assembler check your work.

**What you build, and who uses it later**

| Piece | Objective | Used by |
|---|---|---|
| your drawing of the six formats | 4.1 | you, in 4.3 and 4.4; Lab 5's decoder follows it |
| `Opcode` | 4.2 | Lab 5's `decode()` switches on it |
| `opcode`, `funct3`, `funct7` | 4.3 | `decode()`, to work out which instruction it is |
| `rd`, `rs1`, `rs2` | 4.3 | `decode()`; the register file takes them (Lab 3); the disassembler prints them with `abi_name` (Lab 5) |
| `imm_i`, `imm_s`, `imm_b`, `imm_u`, `imm_j` | 4.4 | `decode()` stores the immediate; Lab 6 adds it to a register value or to the pc |
| `vectors.S`, `rv32i_vectors.hpp` | 4.5 | T4.3 now; T5.1 and T5.3 in Lab 5 |

**Skills practiced**
- `enum class` with a chosen underlying type, and `static_cast` in both directions
- `constexpr`, `noexcept`, `[[nodiscard]]` free functions in a header
- Reading a spec figure and turning it into bit operations
- Assembling and disassembling with the cross toolchain, and reading objdump's output
- Table-driven tests with `TEST_P`, a function pointer as a table column, and an `inline constexpr` table shared between test files

---

## Type rules for this lab

| Quantity | Type | Why |
|---|---|---|
| the instruction | `Word` | a guest value, exactly as `Bus::fetch` returns it |
| rd, rs1, rs2 | `RegIndex` | what `RegisterFile::read` and `write` take (3.1) |
| opcode, funct3, funct7 | `Word` | bit patterns the decoder compares with constants |
| immediates | `Word`, sign-extended | Lab 6 adds them to register values and to the pc |

**Why is a negative immediate a `Word` and not an `SWord`?** Because of what happens to it next. In Lab 6, `sw a2, -4(sp)` computes its address as sp plus the immediate. With sp = 0x800FFFF0 and the immediate -4 stored as 0xFFFFFFFC, unsigned addition gives 0x800FFFEC, and the carry out of bit 31 is simply dropped. That's sp - 4, the right answer. Unsigned arithmetic wraps by definition. Signed arithmetic that overflows is undefined behavior, so the same sum done in `SWord` would be a bug waiting to happen. That's the rule from `hints.md` "General": values stay unsigned, and you convert to signed only at the exact spot an operation needs signed meaning.

**Why do the register extractors need a cast?** If your `RegIndex` is `std::uint8_t` (one of the two choices in the Lab 1 hint), then returning `bits(...)` (a 32-bit `Word`) as a `RegIndex` narrows, and your flags stop the build:

```
error: conversion from 'Word' {aka 'unsigned int'} to ... {aka 'unsigned char'} may change value [-Werror=conversion]
```

Write the `static_cast` yourself. It's safe: a 5-bit field is at most 31, which fits in a byte with room to spare. If your `RegIndex` is `unsigned`, no cast is needed.

---

## Setup

Create the files, from the repository root:

```bash
touch include/rvsim/fields.hpp
touch tests/unit/fields_test.cpp tests/unit/rv32i_vectors.hpp
mkdir -p tests/programs/lab4 && touch tests/programs/lab4/vectors.S
```

Add the test file to the list from Lab 0. There's no library source this time: everything in `fields.hpp` is `constexpr`, and `constexpr` functions live in headers.

`tests/CMakeLists.txt`

```cmake
add_executable(unit_tests
  # ... the test files from Labs 0 to 3 ...
  unit/fields_test.cpp
)
```

Neither header gets listed. `fields.hpp` is found through `include/`, like every header since Lab 1, and `rv32i_vectors.hpp` sits in the same folder as the test that includes it with `#include "rv32i_vectors.hpp"` (quotes search the including file's own folder first).

Empty files compile, so the build stays green while you work through the parts. Build and test as in Lab 0:

```bash
cmake --build build/debug -j
ctest --test-dir build/debug --output-on-failure
ctest --test-dir build/debug --output-on-failure -R Fields   # only tests whose names contain "Fields"
```

---

## Part 1: Reading the spec and drawing the formats (4.1)

### The idea

There are only six layouts, and two promises hold across all of them:

1. **Shared boxes never move.** opcode, rd, rs1, rs2, and funct3 are at the same bit positions in every format that has them. The 4.3 table gives those positions, and they're the same for every instruction.
2. **The immediate's sign bit is always bit 31.** Whatever the format, the topmost bit of the word decides whether the immediate is negative.

Everything else (where the rest of the immediate goes) follows from keeping those two promises. Hardware likes this: it can start reading registers and sign-extending before it has even worked out the format. You get the benefit too: one extractor per field, and one rule for the sign.

### Where to look

- The unprivileged spec: "The RISC-V Instruction Set Manual, Volume I", on riscv.org under Specifications. Any recent version is fine: the formats and the 40 RV32I instructions haven't changed.
- In the RV32I chapter:
  - **"Base Instruction Formats"** draws R, I, S, and U.
  - **"Immediate Encoding Variants"** adds B and J, and has a second figure showing, for each bit of each immediate, which instruction bit it comes from. That second figure is the most useful page for 4.4.
  - Skim the instruction sections that follow ("Integer Computational Instructions", "Control Transfer Instructions", "Load and Store Instructions", "Memory Ordering Instructions", "Environment Call and Breakpoints") to see which format each instruction uses.
- Near the end of the spec, the chapter **"RV32/64G Instruction Set Listings"** has a table with every instruction's opcode, funct3, and funct7 in binary. The RV32I part of it is the 40 instructions in the 4.2 table.

### What to draw

On squared paper, 32 columns per format, bit 31 on the left (the spec's convention). For each of the six formats:

- draw a box for each field and write its bit positions above it;
- label each box with its field name (`rd`, `funct3`, ...);
- label each immediate piece with the immediate bits it holds, the way the spec does: `imm[11:5]` means "this box holds immediate bits 11 down to 5".

The `add` diagram in `labs.md` is the R-type row. Draw it first, then put the other five under it, so boxes at the same bits line up vertically.

### Questions your drawing should answer

1. Which fields sit at the same bit positions in every format that has them?
2. Where is the sign bit of the immediate in each format? (One answer covers all five.)
3. In S-type, what's in bits 11 to 7, where other formats keep rd? Why would the designers do that?
4. B-type looks almost like S-type. Which instruction bits hold different immediate bits in the two formats?
5. Which immediate bit do B and J never store, and why is it safe to leave out?
6. In J-type, which immediate bits are in instruction bits 19 to 12? Which other format keeps immediate bits in exactly that spot?
7. FENCE, ECALL, and EBREAK: whose layout do they borrow, and what's in their immediate bits?

### Check

Decode two words by hand, using nothing but your drawing: `beq a0, a1, +8` is 0x00B50463, and `jal ra, +16` is 0x010000EF. Write out the 32 bits, mark your boxes on them, read each immediate piece, and put the immediate back together. You should get 8 and 16. If you don't, the drawing is wrong, and finding out now is much cheaper than finding out in 4.4. The `labs.md` table has more words with known answers.

The other direction works too: encode an instruction by hand from your drawing, put it in a `.S` file as `.word 0x...`, and see whether objdump (Part 5) decodes it to the instruction you meant.

---

## Part 2: Opcode (4.2)

### The idea

The opcode (bits 6 to 0) is the first box the decoder reads. It says which family the instruction belongs to (a load, a branch, an operation on two registers, ...) and therefore which format to read the rest with. RV32I uses 11 of the 128 possible 7-bit values. Without names, Lab 5's decoder is full of `case 0x63:`. With an enum class, it says `case Opcode::BRANCH:`.

### Syntax reminder, on an unrelated enum

```cpp
enum class HttpStatus : std::uint16_t {   // ": std::uint16_t" is the underlying type
  OK = 200,
  NOT_FOUND = 404,
};

std::uint16_t code = 404;                                                // a number from somewhere
bool missing = code == static_cast<std::uint16_t>(HttpStatus::NOT_FOUND);  // enum to number
HttpStatus status = static_cast<HttpStatus>(code);                       // number to enum
```

- **Scoped names.** `NOT_FOUND` on its own doesn't compile; `HttpStatus::NOT_FOUND` does. Two enum classes can both have an `OK`.
- **No silent conversions.** `code == HttpStatus::NOT_FOUND` fails with `no match for 'operator==' (operand types are 'uint16_t' ... and 'HttpStatus')`. You write the `static_cast`, so every conversion is visible in the code.
- **The underlying type** is the integer type each value is stored as. Leave it out and it's `int`. For `Opcode`, choose `Word`, the type `opcode()` returns in 4.3. Converting between the two is then a cast between identical types, with nothing widened, narrowed, or changing sign. `std::underlying_type_t<Opcode>` (from `<type_traits>`) names it, so a `static_assert` can check it.
- **Binary literals.** The listing table prints opcodes in binary, so you can write them the same way: `0b0110011` is 0x33. A digit separator `'` can group the bits to match the hex digits: `0b011'0011`.
- **Any 7-bit value converts.** `static_cast<Opcode>(0)` compiles, and the result is a perfectly valid `Opcode` that matches none of the 11 names. So Lab 5's switch still needs a path for "none of these", which becomes an illegal instruction. That's also why `opcode()` returns a `Word`: turning the bits into an `Opcode` is a decision, and it belongs to the decoder.

### Scaffold: `include/rvsim/fields.hpp`

```cpp
#pragma once

#include "rvsim/bits.hpp"
#include "rvsim/types.hpp"

namespace rvsim {

// TODO 4.2: enum class Opcode, with underlying type Word and 11 enumerators

// TODO 4.3: opcode(word), rd(word), funct3(word), rs1(word), rs2(word), funct7(word)

// TODO 4.4: imm_i(word), imm_s(word), imm_b(word), imm_u(word), imm_j(word)

}  // namespace rvsim
```

### Gotchas

- **A hyphen isn't part of a name.** `OP-IMM` reads as `OP - IMM`, a subtraction. Write `OP_IMM` and `MISC_MEM`.
- **ALL_CAPS names and macros.** Macros are traditionally ALL_CAPS too, and the preprocessor replaces a macro name everywhere, before the compiler sees your code. If a header ever did `#define LOAD 3`, your enumerator line would become `3 = 0x03` and fail with `expected identifier before numeric constant`, followed by `note: in expansion of macro 'LOAD'`, which gives the game away. None of the headers you use defines these 11 names. This is why some style guides keep ALL_CAPS for macros only.

### Check

At the top of `fields_test.cpp`, one `static_assert` per enumerator, comparing `static_cast<Word>(Opcode::...)` with the hex value from the table, and one more checking the underlying type with `std::is_same_v`. If a value is wrong, the test file doesn't compile.

---

## Part 3: Field extractors (4.3)

### The idea

Shared boxes never move (Part 1), so each extractor does the same job for every instruction: cut its box out with `bits` from Lab 1, using the positions in the 4.3 table. That's the whole function. Lab 1 did the hard work.

### Notes

- Mark each one `constexpr`, `noexcept`, and `[[nodiscard]]`, as in Lab 1 (1.5). There's nothing to `assert`: every `Word` is a valid input.
- A `constexpr` function is implicitly `inline`, which is what lets you define it in a header that many `.cpp` files include. A plain function defined in a header gives a `multiple definition` link error as soon as two `.cpp` files include it.
- Extract a field for every word, even when the instruction's format doesn't have that field. For `addi a0, zero, -1`, `rs2` returns 31: those bits are the top of the immediate. Deciding which fields mean something is Lab 5's job.
- The register extractors return `RegIndex`, through a `static_cast` (see "Type rules" above).

### Gotcha: a variable named like the function

An unrelated example:

```cpp
int area(int w, int h);

int main() {
  int area = area(3, 4);   // error: 'area' cannot be used as a function
}
```

The new variable `area` is in scope from its own `=` onward, so the call on the right sees the variable, not the function. The same happens with `RegIndex rd = rd(word);`, and in Lab 5, inside any class that has a member named `rd`. Either qualify the call, `rvsim::rd(word)`, or give the variable another name.

Also, the fmash16 post's text calls rd, rs1, and rs2 4-bit fields. They're 5 bits (there are 32 registers).

### Check (part of T4.3)

The 4.3 examples: `add a2, a0, a1`, `sub a3, a1, a0` (funct7 0x20), and `addi a0, zero, -1`, whose rs2 and funct7 come from the immediate (31 and 0x7F). One more is worth adding: on the all-ones word 0xFFFFFFFF, every extractor returns its field's largest value (0x7F, 31, 7, 31, 31, 0x7F), so a field cut one bit too narrow shows up.

---

## Part 4: Immediate builders (4.4)

### The idea

An immediate is a constant written into the instruction itself: the -1 in `addi a0, zero, -1`, the -4 in `sw a2, -4(sp)`, the +8 in `beq a0, a1, +8`. Its bits are in the word, but where depends on the format, and in S, B, and J they're split into pieces. It's like a phone number printed on a ticket in pieces, the area code in one corner and the rest in another: to dial it, you read each piece and put it in its place.

Every builder does the same two steps:

1. **Gather.** Cut each piece out with `bits`, shift it left so its lowest bit lands on the immediate bit it holds, and OR the pieces together.
2. **Extend.** Call `sign_extend` with the immediate's width, so a negative immediate becomes a negative 32-bit value.

| Builder | Pieces | Width for `sign_extend` |
|---|---|---|
| `imm_i` | 1 | 12 |
| `imm_s` | 2 | 12 |
| `imm_b` | 4, plus a 0 in bit 0 | 13 |
| `imm_u` | 1, already in place | none: see below |
| `imm_j` | 4, plus a 0 in bit 0 | 21 |

### The technique, on an unrelated format

A made-up 16-bit "stamp" stores a signed 6-bit number `v` in two pieces: stamp bits 15 to 13 hold v's bits 5 to 3, and stamp bits 2 to 0 hold v's bits 2 to 0.

```cpp
constexpr Word stamp_value(Word stamp) {
  const Word v = (bits(stamp, 15, 13) << 3)   // this piece starts at v's bit 3
               | bits(stamp, 2, 0);           // this piece starts at v's bit 0
  return sign_extend(v, 6);                   // v is 6 bits wide
}
```

Every builder has this shape: each piece is shifted by the immediate bit where that piece starts, not by where it sits in the instruction. Your 4.1 drawing supplies the numbers.

### Why the pieces are shuffled

Remember the two promises from Part 1. Once the register boxes are pinned in place and the sign bit is pinned to bit 31, the remaining immediate bits have to fit around them. S keeps the I-type immediate but moves its low 5 bits into the slot where rd would be. B and J don't need bit 0 (offsets are even), and their remaining bits are arranged so that as many as possible line up with S (for B) and with I and U (for J). The fewer places an immediate bit can come from, the less hardware it takes to select it.

### U-type

There's no extension step: the top bit of the U immediate is already bit 31 of the word, the sign bit of a 32-bit value. Keep bits 31 to 12 where they are and make bits 11 to 0 zero. `lui a4, 0x12345` gives 0x12345000. (The fmash16 post's U mask is a typo that keeps some of the low 12 bits.)

### Gotchas

- **The wrong width.** B and J are sign-extended from 13 and 21, not 12 and 20. The immediate is one bit wider than the bits stored, because bit 0 is an implied 0. Extend B from 12 and every offset from 2048 to 4094 comes out negative. Lab 1's T1.2 tested widths 13 and 21 for this reason.
- **Shift by the immediate position, not the instruction position.** A piece that holds immediate bits 4 to 1 shifts left by 1, wherever it sits in the word.
- **Two bits with different jobs.** Instruction bit 7 doesn't mean the same thing in S and in B, and instruction bit 20 doesn't mean the same thing in I and in J. Most B and J bugs are on one of those two bits. Check both against your drawing.
- **SRAI's immediate isn't its shift amount.** `srai a2, a3, 31` is 0x41F6D613, and `imm_i` returns 0x41F, not 31. Bit 10 of the immediate (instruction bit 30, the same bit that turns ADD into SUB) is what marks SRAI; the shift amount is the low 5 bits. Splitting them is Lab 5's job, so 0x41F is the right answer here. In the same way, `imm_i` of `fence iorw,iorw` (0x0FF0000F) is 0x0FF: FENCE keeps its own fields in the immediate bits.

### Check (T4.1, T4.2)

The `labs.md` examples table is the start of T4.1. Use the assembler (Part 5) to find the words for the cases it doesn't have. One `TEST_P` can run every row for all five builders if the row carries the builder itself. A function's name without parentheses turns into a pointer to it, so the builder can be a table column. On an unrelated example:

```cpp
struct MathCase {
  std::string_view name;   // for failure messages and test names
  int (*fn)(int);          // a pointer to any function that takes an int and returns an int
  int input;
  int expected;
};

int twice(int x) { return 2 * x; }
int negate(int x) { return -x; }

const MathCase mathCases[] = {
  {"twice", twice, 3, 6},
  {"negate", negate, 3, -3},
};
```

Your builders all take a `Word` and return a `Word`, so they fit one column type. They're `noexcept`, so the exact pointer type is `Word (*)(Word) noexcept`. A plain `Word (*)(Word)` accepts them too.

A few rows also make good `static_assert`s at the top of the test file, as in T1.1: the builders are `constexpr`, so a wrong one fails the build.

For T4.2, try the B and J builders on every B and J word you have, the backward ones included; on the all-ones word 0xFFFFFFFF, where both must return 0xFFFFFFFE (-2) and not -1; and on words with only bit 7 or only bit 20 set. Bit 0 of the result must be 0 every time.

---

## Part 5: Test vectors (4.5)

### The idea

Your tests need answers you didn't work out yourself. If you compute the expected values with the same understanding you used to write the builders, a misunderstanding ends up in both, and the test passes anyway. The cross assembler is an independent judge: you write instructions in assembly, it encodes them, and objdump decodes the words back to text. Your table copies both sides, the word going in and the fields coming out, and T4.3 checks that your functions agree with the toolchain.

### Writing `vectors.S`

The shape, with lines taken from the `labs.md` examples:

```asm
        .text
        .globl _start
_start:
        addi    a0, zero, -1            # I-type, small negative
        beq     a0, a1, . + 8           # B-type, 8 bytes forward
        jal     zero, . - 1048576       # J-type, most negative offset
```

- **All 40 instructions** from the 4.2 table, at least once each. Group them by format, with a comment saying what each line is for.
- **Every immediate type** at zero, small positive, small negative, largest positive, and most negative. Two more per type are worth having: one with every other bit set (0x555, or 0xAAA as -1366 for I and S), and its opposite pattern. Those put a 1 next to a 0 in every immediate bit, so a piece moved over by one bit can't hide.
- **x0 and x31** (`zero` and `t6`) in rd, rs1, and rs2, somewhere. Use different registers within one instruction, so swapped fields show up.
- **Branch and jump targets relative to `.`**, the address of the instruction itself. `beq a0, a1, . + 4094` is the largest B offset, without placing any code 4 KiB away: the assembler only needs the distance. The file is never run, so it doesn't matter that the target is outside it.
- **U operands are the upper 20 bits**, from 0x0 to 0xfffff: `lui a4, 0x12345`.
- **No pseudo-instructions.** `li`, `mv`, `j`, `ret`, `nop`, `beqz`, and `call` are aliases that can turn into a different instruction, or two. Write the real instruction: `jalr zero, 0(ra)`, not `ret`.
- **Leave `.word` out of the table.** `.word 0x00b50463` places that exact word in the file, which is handy for testing your drawing, but the table should only hold instructions the assembler encoded from text.

### Assembling and dumping

```bash
mkdir -p build/lab4
riscv64-unknown-elf-as -march=rv32i -mabi=ilp32 -o build/lab4/vectors.o tests/programs/lab4/vectors.S
riscv64-unknown-elf-objdump -d -M no-aliases build/lab4/vectors.o > build/lab4/vectors.dump
```

The object is assembled, never linked or run, so its first instruction is at address 0. `build/` is ignored by git. Add `-M no-aliases,numeric` to a second dump to see `x12` instead of `a2`, which helps when you fill in the register columns. Keep the ABI names in the table's text, though: Lab 5's disassembler prints ABI names.

### Reading objdump's output

```
  44:   00b50463                beq     a0,a1,4c <_start+0x4c>
```

| Part | Meaning |
|---|---|
| `44:` | the address, in hex |
| `00b50463` | the word |
| `beq` | the mnemonic |
| `a0,a1,4c` | the operands; for a branch or jump the last one is the target address, in hex |
| `<_start+0x4c>` | the target as label plus offset: ignore it |

objdump separates the parts with tabs, so the spacing depends on your terminal.

Getting the immediate for your row from the text:

| objdump shows | The immediate in your row |
|---|---|
| `addi a0,zero,-1` | -1, which is 0xFFFFFFFF (objdump prints these in decimal) |
| `lw zero,-4(t6)` | -4, and rs1 is t6 |
| `lui a4,0x12345` | 0x12345000 (objdump prints only the upper 20 bits) |
| `beq a0,a1,4c` at 0x44 | 0x4C - 0x44 = 8 |
| `bge s4,s5,fffff058` at 0x58 | 0xFFFFF058 - 0x58 = 0xFFFFF000, which is -4096: subtract in 32 bits and let it wrap |
| `srai a2,a3,0x1f` | 0x41F: the shift amount objdump prints, plus bit 10, which marks SRAI (Part 4) |
| `fence iorw,iorw` | 0x0FF |
| `ecall`, `ebreak` | 0 and 1 |

### Gotchas: the assembler doesn't always stop you

These were all checked with the cross toolchain from Lab 0 (binutils 2.35):

- **An out-of-range branch silently becomes two instructions.** `beq a0, a1, . + 4096` is 2 bytes past the limit. It assembles without a word of warning into `bne a0,a1` jumping over a `jal`. Your table would get an extra row and the wrong instruction. Rule: every instruction line in `vectors.S` must give exactly one line in the dump.
- **An out-of-range jal silently wraps.** `jal ra, . + 1048576`, also 2 bytes past the limit, comes out as an offset of -1048576. Always compare the target objdump prints with the one you meant.
- **An odd offset silently loses bit 0.** `beq a0, a1, . + 3` comes out as `. + 2`.
- Some mistakes do stop the build: `addi a0, a0, 2048` gives `illegal operands`, and `lui a0, -1` gives `lui expression not in range 0..1048575` (write `0xfffff`).
- **Comments that aren't yours.** After some loads, stores, and jalrs, objdump adds something like `# 7fffe800 <_start+0x7fffe800>`. That's objdump guessing the address from earlier `lui` and `auipc` lines in the file. Ignore everything after `#`.
- **`fence.tso` can't be read back.** It assembles, but older objdumps, 2.35 included, print it as a raw word (`0x8330000f`). Leave it out; plain `fence` covers the opcode.
- **Numeric local labels look odd.** A label written `1:` shows up as `.L1^B1` in the dump. Use `.`-relative targets or named labels.

### The shared table: `rv32i_vectors.hpp`

You'll type around 60 rows once. T4.3 checks the fields against them now, and in Lab 5, T5.1 and T5.3 check decoding and disassembly against the same rows. So the table goes in a header that both test files include, not inside `fields_test.cpp`.

A table in a header, on an unrelated example:

```cpp
// planets.hpp
#pragma once

#include <string_view>

struct Planet {
  std::string_view name;
  double gravity;   // m/s^2
};

inline constexpr Planet planets[] = {
  {"Mercury", 3.7},
  {"Earth", 9.8},
};
```

Any test file can `#include "planets.hpp"` and loop over `planets`.

- **`constexpr`**: the table is built at compile time, and it can be used in `static_assert`s.
- **`inline`**: a variable defined in a header that two `.cpp` files include is defined twice. `inline` (C++17) tells the linker it's one variable. Without `inline`, a `constexpr` variable still links, because each file quietly gets its own private copy. `inline constexpr` is how you say "one shared table".
- **`std::string_view`** for the text: it points at the string literal and allocates nothing.

What a row holds: the address, the word, objdump's text (one space after the mnemonic, no `<...>` and no `#` comment), the format, the opcode, rd, rs1, rs2, funct3, funct7, and the immediate. A small `enum class Format { R, I, S, B, U, J };` in the same header says which fields and which builder apply to a row. A field the format doesn't have: write 0 and don't check it.

### Scaffold: `tests/unit/rv32i_vectors.hpp`

```cpp
#pragma once

#include <ostream>
#include <string_view>

#include "rvsim/types.hpp"

// TODO 4.5: enum class Format, one enumerator per format

// TODO 4.5: struct TestVector: pc, word, text, format, opcode, rd, rs1, rs2, funct3, funct7, imm

// TODO 4.5: PrintTo(const TestVector&, std::ostream*), marked inline

// TODO 4.5: inline constexpr TestVector rv32iVectors[] = { one row per objdump line };
```

### Gotchas

- **Every row lists every member, in order.** Leave the last one off and the build stops with `missing initializer for member 'TestVector::imm' [-Werror=missing-field-initializers]`, from `-Wextra`. It's the same rule as the designated initializers in Lab 3.
- **`std::string` works, until it doesn't.** In a `constexpr` table, `std::string` compiles for short texts and then fails once one is longer than 15 characters (`sw s10,-1366(s11)` is 17), with `... is not a constant expression because it refers to a result of 'operator new'`. Short strings fit inside the `std::string` object itself; longer ones need heap memory, which can't outlive compilation. `std::string_view` never allocates.
- **`PrintTo` in a header must be `inline`**, for the same reason as the table. GoogleTest uses it in failure messages and in each row's CTest name. CTest names drop spaces, so `add a2,a0,a1` shows up as `adda2,a0,a1`. Print the spaces as underscores to keep the names readable.

### The table-driven test

```cpp
class TestingVectors : public ::testing::TestWithParam<TestVector> {};

TEST_P(TestingVectors, MatchesObjdump) {
  const TestVector& v = GetParam();
  // TODO T4.3: check the opcode, for every row
  // TODO T4.3: switch on v.format; check the fields that format has, and its immediate
}

INSTANTIATE_TEST_SUITE_P(Vectors, TestingVectors, ::testing::ValuesIn(rv32iVectors));
```

Write the switch over `Format` with no `default:`. Then `-Wswitch-enum` tells you if you forgot a format.

Two cheap checks on the table itself catch copying mistakes: the rows' addresses go up by 4 from 0 (no line skipped or copied twice), and together the rows cover all 40 mnemonics (the text up to the first space).

### Check (T4.3)

Every row passes. Then break something on purpose: swap two pieces in `imm_b`, rebuild, and watch what fails. It should be tests that use `imm_b` (the B rows, and your T4.1 and T4.2 cases for it), and nothing else. Put it back. If nothing fails, your vectors aren't testing what you think.

---

## Expected shape

When everything passes, these hold:

```
imm_i(0xfff00513) = 0xffffffff   addi a0, zero, -1
imm_s(0xfec12e23) = 0xfffffffc   sw a2, -4(sp)
imm_b(0x80b50063) = 0xfffff000   beq a0, a1, -4096: the most negative B offset
imm_u(0x12345737) = 0x12345000   lui a4, 0x12345
imm_j(0x7ffff0ef) = 0x000ffffe   jal ra, +1048574: the largest J offset
imm_b(0xffffffff) = 0xfffffffe   all ones: -2, because bit 0 is never stored
imm_j(0xffffffff) = 0xfffffffe
imm_i(0x41f6d613) = 0x0000041f   srai a2, a3, 31: the shift amount is the low 5 bits

fields of 0xfff00513, addi a0, zero, -1:
opcode 0x13   rd 10   funct3 0   rs1 0   rs2 31   funct7 0x7f
```

---

## Progress checklist

**Reading**
- [ ] 4.1 six formats drawn, every immediate piece labeled; two words decoded by hand

**`fields.hpp`**
- [ ] 4.2 `Opcode`: 11 enumerators, underlying type `Word`, values from the listing table
- [ ] 4.3 six extractors; the register ones return `RegIndex`
- [ ] 4.4 five builders: sign-extended, B and J bit 0 clear, U's low 12 bits zero

**Vectors**
- [ ] 4.5 `vectors.S`: all 40 instructions, every immediate type at zero, small values, and both limits, x0 and x31 in every register field
- [ ] 4.5 one dump line per source line; table copied into `rv32i_vectors.hpp`

**Tests**
- [ ] T4.1 every builder at zero, small positive, small negative, and both limits
- [ ] T4.2 B and J bit 0 always clear: backward offsets, the all-ones word
- [ ] T4.3 every vector's opcode, fields, and immediate
- [ ] Everything passes in the sanitizer build

---

## Stretch work

1. **Encoders and an exhaustive check.** Write the inverse functions, `encode_b(offset)` and `encode_j(offset)`, which put an offset's bits into an instruction word. Then round-trip every even B offset from -4096 to 4094 through `imm_b` (4096 values) and every J offset through `imm_j` (about a million, still well under a second). Checking every value beats checking examples.
2. **The whole table at compile time.** The table and the builders are both `constexpr`, so a `constexpr` function that loops over `rv32iVectors` and returns whether every row matches can sit inside one `static_assert`. A wrong row then fails the build instead of a test.
3. **Generate the rows.** A short script (Python, or awk) that reads the dump and prints the table rows, so changing `vectors.S` never means retyping. Keep the expected values coming from objdump's text, not from your own builders, or the test would check nothing.
4. **`to_string(Opcode)`** returning the spec's names, with a switch over all 11 and no `default:`, so `-Wswitch-enum` flags a missing one. Lab 5's error messages can use it.

---

## Reflection

Once the tests pass, answer these. The first one is part of the lab's "done when".

- Where does each bit of a B and a J immediate sit in the instruction? Why is the sign always instruction bit 31, and what would hardware have to wait for if it weren't?
- Why can B and J leave out immediate bit 0 when I and S can't? (Think about what a load's offset or JALR's offset can add up to.)
- Why do the extractors return fields that an instruction doesn't have, instead of checking the format first?
- Why does `opcode()` return a `Word` and not an `Opcode`?
- Why do the builders return a `Word` when an immediate can be negative?
- Why copy expected values from objdump instead of working them out by hand?
