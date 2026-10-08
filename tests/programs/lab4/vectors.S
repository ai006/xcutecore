/* Lab 4 (4.5): test vectors. Every RV32I instruction at least once, with immediates at zero,
   small positive, small negative, and both range limits of every immediate type, and with x0
   and x31 in every register field. Two more immediates per type set every other bit (1010...
   and 0101...), so a piece moved to the wrong place can't hide.

   Branch and jump targets are written relative to "." (the address of the instruction itself),
   so each offset is exact without placing code kilobytes away. The file is never run, so it
   doesn't matter that the targets are outside it.

   No pseudo-instructions (li, mv, j, ret, nop, beqz): they can expand into other instructions.

   Assemble and dump, from the repository root:
     mkdir -p build/lab4
     riscv64-unknown-elf-as -march=rv32i -mabi=ilp32 -o build/lab4/vectors.o tests/programs/lab4/vectors.S
     riscv64-unknown-elf-objdump -d -M no-aliases build/lab4/vectors.o
*/

        .text
        .globl _start
_start:

# ---- U-type: LUI, AUIPC. imm_u is the upper 20 bits in place, low 12 bits zero ----
        lui     a4, 0x12345             # labs.md example: 0x12345000
        lui     zero, 0x0               # zero, rd = x0
        lui     t6, 0x1                 # small positive: 0x00001000, rd = x31
        lui     a0, 0xfffff             # small negative: 0xFFFFF000 (-4096)
        lui     s0, 0x7ffff             # largest positive: 0x7FFFF000
        lui     s1, 0x80000             # most negative: 0x80000000
        auipc   t0, 0x0                 # zero
        auipc   t6, 0xaaaaa             # pattern 1010...
        auipc   ra, 0x55555             # pattern 0101...

# ---- J-type: JAL. imm_j is a byte offset, -1048576 to 1048574, bit 0 always 0 ----
        jal     ra, . + 16              # labs.md example
        jal     zero, . - 16            # labs.md example
        jal     t6, .                   # zero, rd = x31
        jal     a0, . + 2               # only immediate bit 1
        jal     zero, . + 1048574       # largest positive
        jal     ra, . - 1048576         # most negative
        jal     s2, . + 0xaaaaa         # immediate bits 1, 3, 5, ..., 19
        jal     s3, . - 0xaaaac         # immediate bits 2, 4, 6, ..., 20 (20 is the sign)

# ---- B-type: BEQ, BNE, BLT, BGE, BLTU, BGEU. imm_b is a byte offset, -4096 to 4094 ----
        beq     a0, a1, . + 8           # labs.md example
        bne     a0, zero, . - 8         # labs.md example
        beq     zero, t6, .             # zero, rs1 = x0, rs2 = x31
        bne     t6, zero, . + 2         # only immediate bit 1, rs1 = x31, rs2 = x0
        blt     t5, t4, . + 4094        # largest positive
        bge     s4, s5, . - 4096        # most negative
        bltu    a2, a3, . + 0xaaa       # immediate bits 1, 3, 5, ..., 11
        bgeu    a4, a5, . - 0xaac       # immediate bits 2, 4, 6, ..., 12 (12 is the sign)

# ---- S-type: SB, SH, SW. imm_s is -2048 to 2047, split into two pieces ----
        sw      a2, -4(sp)              # labs.md example
        sb      t6, 0(zero)             # zero, rs1 = x0, rs2 = x31
        sh      zero, 8(t6)             # small positive, rs1 = x31, rs2 = x0
        sw      ra, 2047(sp)            # largest positive
        sb      s6, -2048(s7)           # most negative
        sh      s8, 0x555(s9)           # pattern 0101...
        sw      s10, -0x556(s11)        # pattern 1010... (0xAAA as 12 bits)

# ---- I-type loads: LB, LH, LW, LBU, LHU. imm_i is -2048 to 2047 ----
        lb      a0, 0(a1)               # zero
        lh      t6, 4(zero)             # small positive, rd = x31, rs1 = x0
        lw      zero, -4(t6)            # small negative, rd = x0, rs1 = x31
        lbu     t1, 2047(t2)            # largest positive
        lhu     t3, -2048(s0)           # most negative

# ---- JALR (I-type) ----
        jalr    zero, 0(ra)             # what the ret alias means
        jalr    ra, -2048(t6)           # most negative, rs1 = x31
        jalr    t6, 2047(zero)          # largest positive, rd = x31, rs1 = x0

# ---- OP-IMM (I-type): ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI ----
        addi    a0, zero, -1            # labs.md example
        addi    zero, zero, 0           # zero; what the nop alias means
        addi    sp, sp, 16              # small positive
        slti    a1, a2, 2047            # largest positive
        sltiu   a3, a4, -2048           # most negative
        xori    t6, t6, -1              # all ones, rd = rs1 = x31
        ori     s0, s1, 0x555           # pattern 0101...
        andi    a5, a6, -0x556          # pattern 1010...
        slli    t6, zero, 31            # shift amount 31: imm_i is 31
        srli    a0, a1, 1               # shift amount 1
        srai    a2, a3, 31              # shift amount 31: imm_i is 0x41F, bit 10 marks SRAI
        srai    zero, t6, 0             # shift amount 0: imm_i is 0x400

# ---- OP (R-type): ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND ----
        add     a2, a0, a1              # Lab 0
        sub     a3, a1, a0              # Lab 0: funct7 0x20
        sll     t6, zero, t6            # rd = x31, rs1 = x0, rs2 = x31
        slt     zero, t6, zero          # rd = x0, rs1 = x31, rs2 = x0
        sltu    s0, s1, s2
        xor     a4, a5, a6
        srl     t0, t1, t2
        sra     t3, t4, t5              # funct7 0x20
        or      s10, s11, ra
        and     gp, tp, sp

# ---- MISC-MEM: FENCE. I-type layout; the immediate holds the fm, pred, and succ fields ----
        fence                           # means fence iorw,iorw: imm_i 0x0FF
        fence   r, w                    # imm_i 0x021

# ---- SYSTEM: ECALL, EBREAK. I-type layout; only the immediate tells them apart ----
        ecall                           # imm_i 0
        ebreak                          # imm_i 1
