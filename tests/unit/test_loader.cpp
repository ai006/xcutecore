#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

#include "rvsim/bus.hpp"
#include "rvsim/hex_dump.hpp"
#include "rvsim/loader.hpp"
#include "rvsim/memory.hpp"
#include "rvsim/memory_fault.hpp"
#include "rvsim/types.hpp"

using rvsim::AccessType;
using rvsim::Bus;
using rvsim::Memory;
using rvsim::MemoryFault;

namespace {

// The first four instructions of tests/programs/lab0/first.S, in load order: each 32-bit word
// is stored least significant byte first.
constexpr std::array<std::uint8_t, 16> firstFourWords = {
    0x13, 0x05, 0x50, 0x00,  // 0x00500513  addi a0, zero, 5
    0x93, 0x05, 0x70, 0x00,  // 0x00700593  addi a1, zero, 7
    0x33, 0x06, 0xb5, 0x00,  // 0x00b50633  add  a2, a0, a1
    0xb3, 0x86, 0xa5, 0x40,  // 0x40a586b3  sub  a3, a1, a0
};

constexpr Addr ramBase = 0x80000000;

// A bus with the D2 memory map: 1 MiB of memory at 0x80000000. A Bus is move-only, and
// returning it by value moves it, devices included.
Bus make_bus() {
    Bus bus;
    bus.map(ramBase, std::make_unique<Memory>(0x100000));
    return bus;
}

// Runs access() and returns the MemoryFault it throws, or nothing if it doesn't throw.
template <typename F>
std::optional<MemoryFault> catch_fault(F access) {
    try {
        access();
    } catch (const MemoryFault& fault) {
        return fault;
    }
    return std::nullopt;
}

}

// ---- T2.7: the loader places bytes ----

// The 2.7 example: load 13 05 50 00 at 0x80000000, and a fetch there returns 0x00500513,
// the first instruction of first.S.
TEST(TestingLoader, FirstInstructionFetchesBack) {
    Bus bus = make_bus();
    const std::array<std::uint8_t, 4> bytes = {0x13, 0x05, 0x50, 0x00};

    rvsim::load_bytes(bus, ramBase, bytes);

    EXPECT_EQ(bus.fetch(ramBase), Word{0x00500513});  // addi a0, zero, 5
}

// The first four words of first.S, each fetched back from its own address.
TEST(TestingLoader, PlacesEveryByteAtTheRightAddress) {
    Bus bus = make_bus();

    rvsim::load_bytes(bus, ramBase, firstFourWords);

    EXPECT_EQ(bus.fetch(0x80000000), Word{0x00500513});  // addi a0, zero, 5
    EXPECT_EQ(bus.fetch(0x80000004), Word{0x00700593});  // addi a1, zero, 7
    EXPECT_EQ(bus.fetch(0x80000008), Word{0x00B50633});  // add  a2, a0, a1
    EXPECT_EQ(bus.fetch(0x8000000C), Word{0x40A586B3});  // sub  a3, a1, a0
    EXPECT_EQ(bus.fetch(0x80000010), Word{0});           // nothing written past the end
}

// A std::span<const std::uint8_t> parameter takes bytes from anywhere, without copying them.
TEST(TestingLoader, AcceptsVectorsArraysAndPlainArrays) {
    Bus bus = make_bus();
    const std::vector<std::uint8_t> fromVector = {0x11, 0x22};
    const std::array<std::uint8_t, 1> fromArray = {0x33};
    const std::uint8_t fromPlainArray[] = {0x44};

    rvsim::load_bytes(bus, ramBase, fromVector);
    rvsim::load_bytes(bus, ramBase + 2, fromArray);
    rvsim::load_bytes(bus, ramBase + 3, fromPlainArray);

    EXPECT_EQ(bus.read(ramBase, 4), Word{0x44332211});
}

// Loading nothing writes nothing, so it doesn't even need a device.
TEST(TestingLoader, EmptyLoadWritesNothing) {
    Bus bus;

    EXPECT_NO_THROW(rvsim::load_bytes(bus, ramBase, std::span<const std::uint8_t>{}));
}

// Four bytes starting 2 bytes before the end of memory. The third byte has nowhere to go, so
// the bus throws its MemoryFault for that byte, and the two bytes before it stay written.
TEST(TestingLoader, LoadingPastTheEndOfMemoryThrows) {
    Bus bus;
    bus.map(0x80000000, std::make_unique<Memory>(16));
    const std::array<std::uint8_t, 4> bytes = {0xAA, 0xBB, 0xCC, 0xDD};

    const auto fault = catch_fault([&] { rvsim::load_bytes(bus, 0x8000000E, bytes); });
    ASSERT_TRUE(fault.has_value()) << "a load past the end of memory did not throw";
    EXPECT_EQ(fault->getAddr(), Addr{0x80000010});  // the first missing byte
    EXPECT_EQ(fault->getWidth(), std::size_t{1});   // bytes are written one at a time
    EXPECT_EQ(fault->getAccessType(), AccessType::Write);

    EXPECT_EQ(bus.read(0x8000000E, 2), Word{0xBBAA});
}

// ---- T2.8: the hex dump string, exactly ----

// The 2.8 example string, newline included.
TEST(TestingHexDump, OneFullLine) {
    Bus bus = make_bus();
    rvsim::load_bytes(bus, ramBase, firstFourWords);

    EXPECT_EQ(rvsim::hex_dump(bus, ramBase, 16),
              "80000000: 13 05 50 00 93 05 70 00 33 06 b5 00 b3 86 a5 40\n");
}

// A 20-byte dump has two lines, the second with 4 bytes.
TEST(TestingHexDump, ShorterLastLine) {
    Bus bus = make_bus();
    rvsim::load_bytes(bus, ramBase, firstFourWords);

    EXPECT_EQ(rvsim::hex_dump(bus, ramBase, 20),
              "80000000: 13 05 50 00 93 05 70 00 33 06 b5 00 b3 86 a5 40\n"
              "80000010: 00 00 00 00\n");
}

// // Lines start at start and every 16 bytes after it, not at multiples of 16.
TEST(TestingHexDump, UnalignedStart) {
    Bus bus = make_bus();
    rvsim::load_bytes(bus, ramBase, firstFourWords);

    EXPECT_EQ(rvsim::hex_dump(bus, 0x80000002, 18),
              "80000002: 50 00 93 05 70 00 33 06 b5 00 b3 86 a5 40 00 00\n"
              "80000012: 00 00\n");
}

// // A byte prints as a number. 0x41 is 'A', and a std::uint8_t sent straight to a stream would
// // print as that character.
TEST(TestingHexDump, BytesPrintAsNumbersNotCharacters) {
    Bus bus = make_bus();
    bus.write(ramBase, std::uint8_t{0x41});

    EXPECT_EQ(rvsim::hex_dump(bus, ramBase, 1), "80000000: 41\n");
}

// // The line address is always 8 hex digits, zero-padded. Every address above is already 8
// // digits long, so this needs a device mapped low.
TEST(TestingHexDump, LineAddressIsZeroPadded) {
    Bus bus;
    bus.map(0x10, std::make_unique<Memory>(4));
    bus.write(0x10, 4, 0x0A0B0C0D);

    EXPECT_EQ(rvsim::hex_dump(bus, 0x10, 4), "00000010: 0d 0c 0b 0a\n");
}

// // An empty range is an empty string.
TEST(TestingHexDump, ZeroLengthIsEmpty) {
    Bus bus = make_bus();

    EXPECT_EQ(rvsim::hex_dump(bus, ramBase, 0), "");
}

// // hex_dump reads through the bus, so a byte no device holds throws the bus's MemoryFault.
TEST(TestingHexDump, UnmappedByteThrows) {
    Bus bus = make_bus();

    EXPECT_THROW((void)rvsim::hex_dump(bus, 0x800FFFF8, 16), MemoryFault);
}
