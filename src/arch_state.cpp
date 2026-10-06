
#include "rvsim/arch_state.hpp"

#include <cstddef>
#include <format>

namespace rvsim {


std::ostream&
operator << (std::ostream& os, const ArchState& archState) {

    constexpr std::size_t registersPerLine = 4;

    // std::format builds each piece as a string and the stream just prints it, so no
    // std::hex or std::setfill is ever set on `os`. Those flags are sticky: set on the caller's
    // stream, they would turn everything printed after the dump into hex.
    os << std::format("pc  {:08x}\n", archState.pc);

    for (std::size_t i = 0; i < RegisterFile::numRegisters; i++) {
        const regIndex index = static_cast<regIndex>(i);

        // Three spaces between entries, none after the last one on a line.
        if (i % registersPerLine != 0)
            os << "   ";

        // "x" plus the number, left-aligned in 3 characters ("x0 ", "x10"), then the ABI name
        // left-aligned in 4 ("ra  ", "s10 "), then the value as 8 lowercase hex digits.
        os << std::format("x{:<2} {:<4} {:08x}", i, abi_name(index), archState.regs.read(index));

        if (i % registersPerLine == registersPerLine - 1)
            os << '\n';
    }

    // 3.3: prints the pc, then all 32 registers, four per line, each as its number, its ABI name,
    // and its value in 8 hex digits. The first lines for pc 0x80000008, sp 0x800FFFF0, a2 = 12:
    //
    //   pc  80000008
    //   x0  zero 00000000   x1  ra   00000000   x2  sp   800ffff0   x3  gp   00000000
    //   x4  tp   00000000   x5  t0   00000000   x6  t1   00000000   x7  t2   00000000
    //
    // Every line ends in '\n'. It leaves the stream's formatting flags as they were, so printing a
    // state never turns later output into hex.
    return os;
}

}