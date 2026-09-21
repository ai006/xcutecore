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
