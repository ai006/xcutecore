#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

#include "rvsim/bus.hpp"
#include "rvsim/memory.hpp"
#include "rvsim/memory_fault.hpp"
#include "rvsim/types.hpp"

using rvsim::AccessType;
// using rvsim::Addr;
using rvsim::Bus;
using rvsim::Memory;
using rvsim::MemoryFault;
// using rvsim::Word;

// 2.6: BusValue accepts exactly std::uint8_t, std::uint16_t, and std::uint32_t.
static_assert(rvsim::BusValue<std::uint8_t>);
static_assert(rvsim::BusValue<std::uint16_t>);
static_assert(rvsim::BusValue<std::uint32_t>);
static_assert(!rvsim::BusValue<std::uint64_t>);  // 8 bytes is not a bus width
static_assert(!rvsim::BusValue<int>);            // what a plain literal like 0xFF is
static_assert(!rvsim::BusValue<bool>);           // std::unsigned_integral would accept these two
static_assert(!rvsim::BusValue<char16_t>);

// 2.6: what must not compile, checked at compile time. Each concept asks "does this call
// compile?", so these static_asserts fail the build if the answer ever changes.
template <typename T>
concept CanTypedWrite = requires(Bus& bus, T value) { bus.write(Addr{0}, value); };

template <typename T>
concept CanTypedRead = requires(Bus& bus) { bus.read<T>(Addr{0}); };

static_assert(CanTypedWrite<std::uint8_t>);
static_assert(CanTypedWrite<std::uint16_t>);
static_assert(CanTypedWrite<std::uint32_t>);
static_assert(!CanTypedWrite<int>);  // bus.write(0x80000004, 0xFF) must not compile
static_assert(!CanTypedWrite<bool>);
static_assert(CanTypedRead<std::uint8_t>);
static_assert(CanTypedRead<std::uint16_t>);
static_assert(CanTypedRead<std::uint32_t>);
static_assert(!CanTypedRead<std::uint64_t>);  // bus.read<std::uint64_t>(...) must not compile

namespace {

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

// Maps a new Memory of `size` bytes at base, and returns a raw pointer so the test can look
// inside the device after the bus has taken it. After std::move the unique_ptr is null, so the
// pointer has to be saved first. The bus owns the memory; the raw pointer only looks at it,
// and it stays valid as long as the bus lives.
Memory* map_memory(Bus& bus, Addr base, std::size_t size) {
    std::unique_ptr<Memory> memory = std::make_unique<Memory>(size);
    Memory* view = memory.get();
    bus.map(base, std::move(memory));
    return view;
}

}

// ---- T2.5: routing, unmapped addresses, running off a device, fetch faults, overlaps ----

// With two devices mapped, each address reaches the right device at the right offset.
// Checked through the Memory objects themselves, not only through the bus.
TEST(TestingBus, EachAddressReachesTheRightDeviceAndOffset) {
    Bus bus;
    Memory* ram = map_memory(bus, 0x80000000, 0x100);
    Memory* other = map_memory(bus, 0x10000000, 0x10);

    // Bus writes land in the right device, at address - base.
    bus.write(0x80000010, 4, 0x11223344);
    EXPECT_EQ(ram->read(0x10, 4), Word{0x11223344});
    bus.write(0x10000004, 1, 0xAB);
    EXPECT_EQ(other->read(0x4, 1), Word{0xAB});
    EXPECT_EQ(ram->read(0x4, 1), Word{0});  // the same offset in the other device is untouched

    // Bus reads come back from the right device.
    ram->write(0xFC, 4, 0xCAFEF00D);
    other->write(0x0, 2, 0xBEEF);
    EXPECT_EQ(bus.read(0x800000FC, 4), Word{0xCAFEF00D});
    EXPECT_EQ(bus.read(0x10000000, 2), Word{0xBEEF});
    EXPECT_EQ(bus.read(0x10000001, 1), Word{0xBE});
}

// The 2.5 example, with 1 MiB of memory at 0x80000000.
TEST(TestingBus, OneMebibyteAtTheRamBase) {
    Bus bus;
    Memory* ram = map_memory(bus, 0x80000000, 0x100000);

    bus.write(0x80000010, 4, 0x12345678);
    EXPECT_EQ(ram->read(0x10, 4), Word{0x12345678});  // 0x80000010 arrives as offset 0x10

    EXPECT_NO_THROW(bus.write(0x800FFFFC, 4, 0xDEADBEEF));  // the last valid word
    EXPECT_EQ(ram->read(0xFFFFC, 4), Word{0xDEADBEEF});

    EXPECT_THROW((void)bus.read(0x800FFFFE, 4), MemoryFault);  // runs off the end
    EXPECT_THROW((void)bus.read(0x80100000, 4), MemoryFault);  // just past the end
    EXPECT_THROW((void)bus.read(0x7FFFFFFC, 4), MemoryFault);  // just below the base
}

// Addresses that no device holds throw: on an empty bus, and all around a mapped device.
TEST(TestingBus, UnmappedAddressesThrow) {
    Bus empty;
    EXPECT_THROW((void)empty.fetch(0x80000000), MemoryFault);
    EXPECT_THROW((void)empty.read(0x00000000, 1), MemoryFault);
    EXPECT_THROW(empty.write(0xFFFFFFFF, 1, 0), MemoryFault);

    Bus bus;
    map_memory(bus, 0x80000000, 0x100);
    EXPECT_THROW((void)bus.read(0x00000000, 4), MemoryFault);
    EXPECT_THROW((void)bus.read(0x7FFFFFFF, 1), MemoryFault);  // one byte below the base
    EXPECT_THROW((void)bus.read(0x80000100, 1), MemoryFault);  // one byte past the end
    EXPECT_THROW((void)bus.read(0xFFFFFFFF, 1), MemoryFault);
}

// An access that runs off the end of a device throws, even when another device starts right
// after it and every byte of the access exists: an access must fit inside one device.
TEST(TestingBus, AccessMustFitInsideOneDevice) {
    Bus bus;
    Memory* first = map_memory(bus, 0x1000, 0x10);   // 0x1000 to 0x100F
    Memory* second = map_memory(bus, 0x1010, 0x10);  // 0x1010 to 0x101F: touches the first

    const auto fault = catch_fault([&] { (void)bus.read(0x100E, 4); });
    ASSERT_TRUE(fault.has_value()) << "a read that straddles two devices did not throw";
    EXPECT_EQ(fault->getAddr(), Addr{0x100E});
    EXPECT_EQ(fault->getWidth(), std::size_t{4});
    EXPECT_EQ(fault->getAccessType(), AccessType::Read);

    // A write that straddles the boundary changes neither device.
    EXPECT_THROW(bus.write(0x100E, 4, 0xFFFFFFFF), MemoryFault);
    EXPECT_EQ(first->read(0xE, 2), Word{0});
    EXPECT_EQ(second->read(0x0, 2), Word{0});

    // The accesses on either side of the boundary are fine.
    EXPECT_NO_THROW((void)bus.read(0x100C, 4));
    EXPECT_NO_THROW((void)bus.read(0x1010, 4));
}

// Faults carry the bus address, never the device offset, and the access type of the entry
// point that was called: a failed fetch says Fetch.
TEST(TestingBus, FaultsCarryTheBusAddressAndAccessType) {
    Bus bus;
    map_memory(bus, 0x80000000, 0x100000);

    const auto fetchFault = catch_fault([&] { (void)bus.fetch(0x80100000); });
    ASSERT_TRUE(fetchFault.has_value()) << "fetch(0x80100000) did not throw";
    EXPECT_EQ(fetchFault->getAddr(), Addr{0x80100000});
    EXPECT_EQ(fetchFault->getWidth(), std::size_t{4});  // a fetch is always 4 bytes
    EXPECT_EQ(fetchFault->getAccessType(), AccessType::Fetch);
    EXPECT_STREQ(fetchFault->what(), "memory fault: 4-byte fetch at 0x80100000");

    const auto readFault = catch_fault([&] { (void)bus.read(0x80100000, 4); });
    ASSERT_TRUE(readFault.has_value()) << "read(0x80100000, 4) did not throw";
    EXPECT_EQ(readFault->getAccessType(), AccessType::Read);
    EXPECT_STREQ(readFault->what(), "memory fault: 4-byte read at 0x80100000");

    // The bus found a device, but the access runs off its end. The fault still carries the
    // bus address 0x800FFFFE, not the offset 0xFFFFE a Memory would have reported.
    const auto writeFault = catch_fault([&] { bus.write(0x800FFFFE, 4, 0); });
    ASSERT_TRUE(writeFault.has_value()) << "write(0x800FFFFE, 4, 0) did not throw";
    EXPECT_EQ(writeFault->getAddr(), Addr{0x800FFFFE});
    EXPECT_EQ(writeFault->getWidth(), std::size_t{4});
    EXPECT_EQ(writeFault->getAccessType(), AccessType::Write);
}

// Fetch and data access reach the same memory (Von Neumann): bytes written as data can be
// fetched back as an instruction.
TEST(TestingBus, FetchAndDataAccessReachTheSameMemory) {
    Bus bus;
    map_memory(bus, 0x80000000, 0x100);

    bus.write(0x80000000, 4, 0x00500513);  // addi a0, zero, 5
    EXPECT_EQ(bus.fetch(0x80000000), Word{0x00500513});
    EXPECT_EQ(bus.read(0x80000000, 4), bus.fetch(0x80000000));
}

// Overlapping ranges are rejected with std::invalid_argument.
TEST(TestingBus, OverlappingMappingsAreRejected) {
    Bus bus;
    map_memory(bus, 0x80000000, 0x100000);  // 0x80000000 to 0x800FFFFF

    // the 2.5 example: starts inside the memory
    EXPECT_THROW(bus.map(0x800FF000, std::make_unique<Memory>(0x2000)), std::invalid_argument);
    // the same base
    EXPECT_THROW(bus.map(0x80000000, std::make_unique<Memory>(16)), std::invalid_argument);
    // starts below the memory and ends inside it
    EXPECT_THROW(bus.map(0x7FFFFFF0, std::make_unique<Memory>(0x20)), std::invalid_argument);
    // entirely inside it
    EXPECT_THROW(bus.map(0x80001000, std::make_unique<Memory>(16)), std::invalid_argument);
    // covers it entirely
    EXPECT_THROW(bus.map(0x7FF00000, std::make_unique<Memory>(0x300000)), std::invalid_argument);
}

// Touching ranges, where one ends exactly where the next begins, don't overlap.
TEST(TestingBus, TouchingMappingsAreAllowed) {
    Bus bus;
    map_memory(bus, 0x80000000, 0x100000);

    EXPECT_NO_THROW(bus.map(0x80100000, std::make_unique<Memory>(16)));  // starts at its end
    EXPECT_NO_THROW(bus.map(0x7FFFFFF0, std::make_unique<Memory>(16)));  // ends at its base
}

// A range that runs past 0xFFFFFFFF is rejected; one that ends exactly at the top is fine.
TEST(TestingBus, MappingPastTheTopIsRejected) {
    Bus bus;
    EXPECT_THROW(bus.map(0xFFFFFFF8, std::make_unique<Memory>(16)), std::invalid_argument);

    Memory* top = map_memory(bus, 0xFFFFFFF0, 16);  // its last byte is 0xFFFFFFFF
    bus.write(0xFFFFFFFC, 4, 0x01020304);
    EXPECT_EQ(top->read(0xC, 4), Word{0x01020304});
    EXPECT_EQ(bus.read(0xFFFFFFFF, 1), Word{0x01});
    EXPECT_THROW((void)bus.read(0xFFFFFFFE, 4), MemoryFault);  // would need 0x1_0000_0000
}

// Every check runs before the device is added, so a map() that throws leaves the bus
// unchanged: the old device still works, and the rejected range is still unmapped.
TEST(TestingBus, FailedMapLeavesTheBusUnchanged) {
    Bus bus;
    Memory* ram = map_memory(bus, 0x1000, 0x100);  // 0x1000 to 0x10FF

    EXPECT_THROW(bus.map(0x10F0, std::make_unique<Memory>(0x100)), std::invalid_argument);

    EXPECT_THROW((void)bus.read(0x1100, 1), MemoryFault);  // inside the rejected range
    bus.write(0x10F0, 1, 0x5A);
    EXPECT_EQ(ram->read(0xF0, 1), Word{0x5A});
}

// Beyond the spec: a null device, or one that covers no bytes, is a setup mistake too.
TEST(TestingBus, NullAndEmptyDevicesAreRejected) {
    Bus bus;

    EXPECT_THROW(bus.map(0x1000, nullptr), std::invalid_argument);
    EXPECT_THROW(bus.map(0x1000, std::make_unique<Memory>(0)), std::invalid_argument);
}

// ---- 2.6 and T2.6: typed access ----

// The 2.6 examples.
TEST(TestingBus, TypedAccessExamples) {
    Bus bus;
    map_memory(bus, 0x80000000, 16);

    bus.write<std::uint32_t>(0x80000000, 0x12345678);
    EXPECT_EQ(bus.read<std::uint16_t>(0x80000002), std::uint16_t{0x1234});

    // T is deduced from the value, so this is a 1-byte write. The word is filled with 0xEE
    // first: memory starts zeroed, so a write that was too wide would otherwise store zeros
    // over zeros and go unnoticed.
    bus.write(0x80000004, 4, 0xEEEEEEEE);
    bus.write(0x80000004, std::uint8_t{0xFF});
    EXPECT_EQ(bus.read(0x80000004, 4), Word{0xEEEEEEFF});
}

// T2.6: a typed test runs one test body once for each type in a list.
template <typename T>
class TestingBusTyped : public ::testing::Test {

    protected:
        static constexpr Addr base = 0x80000000;
        static constexpr std::size_t size = 64;

        TestingBusTyped() {
            bus.map(base, std::make_unique<Memory>(size));
        }

        Bus bus;
};

using BusValueTypes = ::testing::Types<std::uint8_t, std::uint16_t, std::uint32_t>;
TYPED_TEST_SUITE(TestingBusTyped, BusValueTypes);

TYPED_TEST(TestingBusTyped, RoundTrip) {
    // Every byte of 0x12345678 is different, so a byte-order bug can't hide. Converted to
    // TypeParam (the current type) it's 0x78, 0x5678, or 0x12345678.
    const TypeParam value = static_cast<TypeParam>(0x12345678);
    const Addr address = TestFixture::base + 8;

    // Surround the target with 0xEE first. Memory starts zeroed, so a write that was too wide
    // or in the wrong place would otherwise store zeros over zeros and go unnoticed.
    this->bus.write(address - 4, 4, 0xEEEEEEEE);
    this->bus.write(address, 4, 0xEEEEEEEE);
    this->bus.write(address + 4, 4, 0xEEEEEEEE);

    this->bus.write(address, value);  // TypeParam is deduced from value

    // The test body is a template whose base class depends on TypeParam, so the compiler can't
    // tell that read is a template until it knows TypeParam. `template` says so; without it,
    // GCC reports "expected 'template' keyword before dependent template name".
    EXPECT_EQ(this->bus.template read<TypeParam>(address), value);

    // The write was sizeof(TypeParam) bytes wide: every 0xEE around it survived.
    EXPECT_EQ(this->bus.read(address - 1, 1), Word{0xEE});
    for (Addr i = sizeof(TypeParam); i < 8; i++)
        EXPECT_EQ(this->bus.read(address + i, 1), Word{0xEE}) << "at address + " << i;
}

// The read is sizeof(TypeParam) bytes wide too. A narrower value can't show that (the cast to
// TypeParam would trim a wider read back down), but the end of memory can: at the last address
// where TypeParam fits, a wider access would run off the end and fault.
TYPED_TEST(TestingBusTyped, FitsExactlyAtTheEndOfMemory) {
    const Addr last =
        static_cast<Addr>(TestFixture::base + TestFixture::size - sizeof(TypeParam));
    const TypeParam value = static_cast<TypeParam>(0x12345678);

    EXPECT_NO_THROW(this->bus.write(last, value));
    EXPECT_EQ(this->bus.template read<TypeParam>(last), value);

    EXPECT_THROW((void)this->bus.template read<TypeParam>(last + 1), MemoryFault);
    EXPECT_THROW(this->bus.write(last + 1, value), MemoryFault);
}
