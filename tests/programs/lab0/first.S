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
