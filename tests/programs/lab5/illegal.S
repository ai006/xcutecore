# Lab 5, T5.5: words that decode() must turn into an IllegalInstruction.
#
# Many are real instructions from other parts of the spec (RV64I, RV32M, F, Zicsr, Zifencei,
# the privileged architecture), assembled for an RV64 target that has those extensions, so
# the cross assembler, not a hand calculation, produces each word. The .insn lines build
# encodings no mnemonic has: the assembler packs the fields we name into the format we name.
# The two .word lines are illegal by definition, and the third is a compressed instruction.
#
# Assemble and list it with:
#   mkdir -p build/lab5
#   riscv64-unknown-elf-as -march=rv64imafd -o build/lab5/illegal.o tests/programs/lab5/illegal.S
#   riscv64-unknown-elf-objdump -d -M no-aliases build/lab5/illegal.o
# objdump decodes many of these as the instructions they are in other extensions; for RV32I
# they're all illegal.

    .text
    .globl _start
_start:
    # illegal by definition
    .word 0x00000000                        # all zeros
    .word 0xffffffff                        # all ones

    # opcodes RV32I doesn't use
    .insn r 0x0b, 0, 0, a0, a1, a2          # custom-0
    .insn r 0x7b, 0, 0, a0, a1, a2          # custom-3
    .word 0x00004501                        # c.li a0,0 (RVC): low two bits 01, not 11
    flw     fa0, 0(a1)                      # LOAD-FP, from F
    fadd.s  fa0, fa1, fa2                   # OP-FP, from F
    addiw   a0, a1, 1                       # OP-IMM-32, RV64 only
    addw    a0, a1, a2                      # OP-32, RV64 only

    # JALR needs funct3 000
    .insn i 0x67, 1, ra, a0, 0              # JALR with funct3 001
    .insn i 0x67, 7, ra, a0, 0              # JALR with funct3 111

    # BRANCH funct3 010 and 011 are unused
    .insn b 0x63, 2, a0, a1, _start         # BRANCH funct3 010
    .insn b 0x63, 3, a0, a1, _start         # BRANCH funct3 011

    # LOAD and STORE: RV64's LD, LWU, and SD, and funct3 values nothing uses
    ld      a0, 0(a1)                       # LOAD funct3 011: RV64 LD
    lwu     a0, 0(a1)                       # LOAD funct3 110: RV64 LWU
    .insn i 0x03, 7, a0, a1, 0              # LOAD funct3 111
    sd      a0, 0(a1)                       # STORE funct3 011: RV64 SD
    .insn s 0x23, 4, a0, 0(a1)              # STORE funct3 100
    .insn s 0x23, 7, a0, 0(a1)              # STORE funct3 111

    # OP: funct7 must be 0000000, or 0100000 for SUB and SRA only
    mul     a0, a1, a2                      # funct7 0000001: RV32M MUL
    divu    a0, a1, a2                      # funct7 0000001: RV32M DIVU
    .insn r 0x33, 1, 0x20, a0, a1, a2       # funct7 0100000 with SLL's funct3
    .insn r 0x33, 0, 0x40, a0, a1, a2       # funct7 1000000

    # OP-IMM shifts: the upper seven immediate bits must be 0000000, or 0100000 for SRAI
    slli    a0, a1, 32                      # funct7 0000001: RV64 shift by 32
    srai    a0, a1, 32                      # funct7 0100001: RV64 shift by 32
    .insn i 0x13, 1, a0, a1, 0x400          # SLLI with funct7 0100000
    .insn i 0x13, 5, a0, a1, -1             # SRLI or SRAI with funct7 1111111

    # MISC-MEM funct3 001: FENCE.I, from Zifencei
    fence.i                                 # FENCE.I

    # SYSTEM: only the exact ECALL and EBREAK words are legal
    csrrw   a0, mstatus, a1                 # Zicsr CSRRW
    csrrs   a0, cycle, zero                 # Zicsr CSRRS (rdcycle)
    mret                                    # privileged MRET
    wfi                                     # privileged WFI
    .insn i 0x73, 0, ra, zero, 0            # ECALL with rd = x1
    .insn i 0x73, 0, zero, a0, 0            # ECALL with rs1 = x10
    .insn i 0x73, 0, ra, zero, 1            # EBREAK with rd = x1
