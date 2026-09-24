# Lab 2 hints: Memory and the bus

A walkthrough for Lab 2 in `labs.md`. Part numbers match objective numbers (Part 5 is objective 2.5). Each part explains the idea, the C++ it needs, gives a scaffold with TODOs, lists the gotchas, and says how to check your work. Scaffolds show where things go, not the code: you write every declaration and every body. Try each objective before reading its part.

---

## Overview

You are building the machine's address space. The CPU never touches memory directly. It hands every access to the `Bus`; the bus finds the device that owns the address, subtracts that device's base, and forwards the access. `Memory` is one device, and Lab 8 adds a console.

```
CPU (Lab 7)
 │   fetch(addr)    read(addr, width)    write(addr, width, value)
 ▼
Bus: finds the device that holds addr, subtracts that device's base
 ├── Memory at 0x80000000, 1 MiB        bus address 0x80000010 arrives as offset 0x10
 └── ConsoleDevice at 0x10000000        Lab 8
```

If you did Epic Duel, you know this shape. `Device` is `Character`, `Memory` is `Warrior`, Lab 8's `ConsoleDevice` is `Sorcerer`, and the bus is the loop that calls `characters[i]->attack()` without knowing what each one is. Two upgrades over Epic Duel: the bus owns its devices through `std::unique_ptr` (Epic Duel stretch 3, so there is no Task 19), and `Device` has a virtual destructor from the start.

**What you build, and who uses it later**

| Piece | Objective | Used by |
|---|---|---|
| `Device` | 2.1 | every device: `Memory` now, `ConsoleDevice` in Lab 8 |
| `Memory` | 2.2 to 2.4 | holds every program from Lab 7 on |
| `AccessType`, `MemoryFault` | 2.4 | Lab 7 turns faults into stop reasons |
| `Bus` | 2.5 | the CPU in Labs 7 to 12; the caches in Lab 15 sit in front of it |
| `read<T>`, `write<T>` | 2.6 | tests, the loader, and Lab 6's store instructions |
| `load_bytes` | 2.7 | unit tests from Lab 7 on; Lab 8's file loaders |
| `hex_dump` | 2.8 | you, while debugging |

**Skills practiced**
- Abstract classes, pure virtual functions, virtual destructors, `override`
- Ownership with `std::unique_ptr` and `std::move`
- Custom exceptions derived from `std::runtime_error`
- Member function templates, and writing your own concept
- `std::vector`, `std::span`, stream formatting
- GoogleTest typed tests

---

## Type rules for this lab

| Quantity | Type | Why |
|---|---|---|
| Bus address, device offset | `Addr` | a guest address |
| Value read or written | `Word` | a guest value; 1- and 2-byte accesses use its low bits |
| Size, width, length (a count of bytes) | `std::size_t` | the C++ type for "how many bytes" |

This answers "should `size()` return `int` or `Word`?" Neither.

- Not `int`: a size is never negative, and your flags turn comparisons between an `int` and an unsigned `Addr` into compile errors (`-Wsign-compare` comes with `-Wall`, and `-Werror` makes it fatal).
- Not `Word`: in this project `Word` means a value the guest program sees, like a register or an instruction. A byte count is a number on the host side.
- `std::size_t` (from `<cstddef>`) is what `std::vector::size()`, `std::span::size()`, and `sizeof` return. So `Memory::size()` can return the vector's size with no cast, and 2.6 can pass `sizeof(T)` straight through as a width. It is 64 bits on your machine, so `offset + width` computed in it can't wrap around, which a 32-bit sum can near 0xFFFFFFFF.

Rule of thumb: guest quantities are `Addr` and `Word`; host counts are `std::size_t`.

---

## Setup

Create the files, from the repository root:

```bash
touch include/rvsim/{device,memory_fault,memory,bus,loader,hex_dump}.hpp
touch src/{memory_fault,memory,bus,loader,hex_dump}.cpp
touch tests/unit/{memory,bus,loader}_test.cpp
```

Add the sources and tests to the lists from Lab 0. Headers don't need listing, because `rvcore` already exports `include/`.

`src/CMakeLists.txt`

```cmake
add_library(rvcore STATIC
  version.cpp
  memory_fault.cpp
  memory.cpp
  bus.cpp
  loader.cpp
  hex_dump.cpp
)
```

`tests/CMakeLists.txt`

```cmake
add_executable(unit_tests
  unit/smoke_test.cpp
  unit/bits_test.cpp
  unit/memory_test.cpp
  unit/bus_test.cpp
  unit/loader_test.cpp
)
```

Empty files compile, so the build stays green while you work through the parts. Build and test as in Lab 0:

```bash
cmake --build build/debug -j
ctest --test-dir build/debug --output-on-failure
ctest --test-dir build/debug --output-on-failure -R Memory   # only tests whose names contain "Memory"
```

---

## Part 1: Device (2.1)

### The idea

`Device` is a contract: "I cover `size()` bytes, and you can read or write 1, 2, or 4 of them at an offset." It has no data and no behavior of its own. Its three functions, `size`, `read`, and `write`, are pure virtual, which makes `Device` abstract: you can't create one, only classes derived from it.

The bus holds devices as `std::unique_ptr<Device>` and deletes them through that base pointer. Deleting a derived object through a base pointer is undefined behavior unless the base destructor is virtual (Epic Duel Task 19). So the destructor is virtual even though it does nothing.

### Syntax reminder, on an unrelated class

```cpp
class Shape {
 public:
  virtual ~Shape() = default;       // virtual destructor with a compiler-written body
  virtual double area() const = 0;  // pure virtual: no body; derived classes must override it
};
```

### Decisions built into this interface

- `size()` is `const`: asking for a size changes nothing.
- `read` is not `const`. On real hardware a read can have side effects: a UART's receive register hands you the next byte and removes it. So the bus's `read` isn't `const` either, and `hex_dump` (2.8) takes a `Bus&`.
- Mark `size()` and `read` `[[nodiscard]]`, as in Lab 1: ignoring the result of a read is almost always a bug. Put it on the overrides in `Memory` too, because the attribute isn't inherited: a call on a `Memory` object only sees `Memory`'s declaration. It has one cost in tests; see the gotcha in Part 4.
- Offsets, not addresses: a device never knows where it is mapped. The same `Memory` works at 0x80000000 in the simulator and at any base in a test.

### Should Device be copyable?

No. Copying through base references slices. With `Device& a = ram1; Device& b = ram2;`, the line `a = b;` compiles and copies only the `Device` part, which is nothing, so `ram1` silently stays as it was. Delete the copy constructor and the copy assignment operator in `Device`, and that line stops compiling.

Once you declare any constructor, even a deleted one, the compiler stops writing the default constructor for you. So also write `Device() = default;`. Without it, `Memory`'s constructor fails with `no matching function for call to 'Device::Device()'`.

### Scaffold: `include/rvsim/device.hpp`

```cpp
#pragma once

#include <cstddef>

#include "rvsim/types.hpp"

namespace rvsim {

class Device {
 public:
  // TODO 2.1: default constructor; copy constructor and copy assignment deleted
  // TODO 2.1: virtual destructor, defaulted

  // TODO 2.1: size(), pure virtual and const
  // TODO 2.1: read(offset, width), pure virtual
  // TODO 2.1: write(offset, width, value), pure virtual
};

}  // namespace rvsim
```

There is no `device.cpp`: a class made of pure virtual functions and a defaulted destructor has nothing to define.

### Gotchas

- **`undefined reference to 'vtable for rvsim::Device'`** at link time means some virtual function is declared without `= 0` and never defined anywhere. Add the `= 0`.
- Parameter names are optional in a pure virtual declaration, but write them (`offset`, `width`, `value`). They are the documentation.

### Check (T2.1)

Two compile-time checks from `<type_traits>`, each inside a `static_assert` at the top of `memory_test.cpp`: `std::is_abstract_v<Device>` and `std::has_virtual_destructor_v<Device>`. If either is false, the test file doesn't compile.

---

## Part 2: Memory (2.2)

### The idea

`Memory` is the first real device: a `std::vector<std::uint8_t>` with one element per byte. The vector owns the storage and frees it when the `Memory` is destroyed (RAII), so there is no `new` or `delete` anywhere.

Start with width 1 only. That gets the class compiling and tested before byte order enters the picture.

### Scaffold: `include/rvsim/memory.hpp`

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "rvsim/device.hpp"
#include "rvsim/memory_fault.hpp"  // needed from 2.4 on, for AccessType

namespace rvsim {

class Memory : public Device {  // `final` is a good idea: nothing derives from Memory
 public:
  // TODO 2.2: explicit constructor taking the size in bytes
  // TODO 2.2: size() override
  // TODO 2.2: read() override
  // TODO 2.2: write() override

 private:
  // TODO 2.4: a const helper that checks one access fits
  // TODO 2.2: the storage
};

}  // namespace rvsim
```

### Scaffold: `src/memory.cpp`

```cpp
#include "rvsim/memory.hpp"

#include <cassert>

namespace rvsim {

// TODO 2.2: constructor; build the storage in the member initializer list
// TODO 2.2: size()
// TODO 2.2, 2.3, 2.4: read()
// TODO 2.2, 2.3, 2.4: write()

}  // namespace rvsim
```

### Notes

- `explicit` stops `Memory m = 16;` from compiling. A number shouldn't quietly turn into a memory.
- `override` on all three turns a signature mismatch into a clear error: `marked 'override', but does not override`. Without it, a mismatch (a missing `const` on `size()` is the usual one) quietly declares a new function, `Memory` stays abstract, and the error shows up somewhere else as `cannot declare variable 'm' to be of abstract type 'Memory'`.
- Until 2.3, `assert(width == 1)` at the top of `read` and `write`, so a wider access fails loudly instead of doing something half right.
- Reading: a byte converts to `Word` without a cast; `Word{byte}` is a widening conversion. Writing: storing a `Word` into a `std::uint8_t` narrows, so `-Wconversion` wants an explicit `static_cast<std::uint8_t>(...)`. The cast keeps the low 8 bits, which is exactly the rule for writes.

### Gotchas

- **Braces versus parentheses.** `std::vector<std::uint8_t> v(16)` is sixteen zeros. `std::vector<std::uint8_t> v{16}` is one element holding 16, because braces prefer the initializer-list constructor. Use parentheses for a size.
- A data member can't be sized with parentheses in the class body: `std::vector<std::uint8_t> bytes_(16);` fails with `expected identifier before numeric constant`. Build the vector in the constructor's member initializer list.

### Check (T2.2)

A new 16-byte memory has `size()` 16 and reads 0 at every offset. Write 0xAB at offset 3 and read it back. Write 0x1FF at offset 4: it reads back 0xFF, and offset 5 is still 0.

---

## Part 3: Little-endian access (2.3)

### The idea

RISC-V is little-endian: the least significant byte of a value sits at the lowest address.

```
write(0, 4, 0x12345678)

offset:   0    1    2    3
byte:    78   56   34   12
        low             high
```

For byte `i` of an access, with `i` from 0 to `width - 1`, the byte at `offset + i` holds bits `8i` to `8i + 7` of the value.

- Read: start from 0; for each byte, shift it left by `8i` and OR it in.
- Write: for each byte, shift the value right by `8i` and store the low 8 bits.

One loop over `i` handles all three widths, and misaligned offsets need no extra code. Lab 9's `rv32ui-ma_data` test depends on that.

### Why not `std::memcpy` or a pointer cast?

Casting a pointer into the byte vector to `std::uint32_t*` breaks the strict aliasing and alignment rules, which is undefined behavior. `std::memcpy` is legal but copies in the host's byte order; x86 happens to be little-endian, so it would work by accident. Shifts give RISC-V byte order on any host. If you do use `memcpy`, add `static_assert(std::endian::native == std::endian::little)` (from `<bit>`) and record the assumption in `DESIGN.md`.

### Gotcha: integer promotion

`bytes_[i] << (8 * i)` doesn't shift a `std::uint8_t`: the byte is promoted to a signed `int` first. OR-ing that `int` into a `Word` then fails with `conversion to 'Word' from 'int' may change the sign of the result [-Werror=sign-conversion]`. Convert the byte to `Word` first, then shift. Lab 1 warned about this; this is where it bites.

### Check (T2.3)

Write 0x12345678 at offset 0 and read the four bytes back one at a time: 0x78, 0x56, 0x34, 0x12. Then the 2.3 examples: a 2-byte read at offset 1 gives 0x3456, and a 2-byte write at offset 8 changes offsets 8 and 9 only (fill offset 10 with something nonzero first, so you can see it survive). Finally, round trips at every width at offset 0 and at `size() - width`, the last offset where that width fits.

---

## Part 4: MemoryFault and bounds checks (2.4)

### The idea

A guest program can compute any address, so a bad address is an event in the simulated machine, not a bug in your simulator. It gets an exception, which Lab 7 catches and turns into a stop reason. A bad width can only come from your own code, so it stays an `assert`. Lab 1 drew the same line: a bad bit position is a simulator bug, so it gets an `assert`, never an exception.

### `AccessType`

`enum class AccessType` with `Fetch`, `Read`, and `Write`, in `memory_fault.hpp`. Add a free function `to_string(AccessType)` returning a `std::string_view`; the message needs it. Switch over all three enumerators with no `default:`, so `-Wswitch-enum` tells you if you ever add a fourth.

GCC still reports `control reaches end of non-void function` after a switch that returns in every case, because an enum variable can hold values outside its enumerators. Put one `return` after the switch.

### `MemoryFault`

Derive publicly from `std::runtime_error` (`<stdexcept>`). That gives you `what()`, and code that catches `std::exception` catches yours too.

- The constructor takes the address, the width, and the access type, and stores them in private members. The getters return them; make them `const` and `noexcept`.
- The message goes to the base class, in the member initializer list. The base is built before your members, so you can't build the message from them. Build it in a private `static` member function that takes the three constructor parameters and returns a `std::string`.
- Formatting: a `std::ostringstream` (`<sstream>`) with `std::hex`, `std::setw(8)`, and `std::setfill('0')` (`<iomanip>`) prints 13 as `0000000d`. With GCC 13 you can use `std::format("{:08x}", addr)` instead.
- The width prints in decimal, so write it before switching the stream to `std::hex`.

### Scaffold: `include/rvsim/memory_fault.hpp`

```cpp
#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

#include "rvsim/types.hpp"

namespace rvsim {

// TODO 2.4: enum class AccessType
// TODO 2.4: to_string(AccessType)

class MemoryFault : public std::runtime_error {
 public:
  // TODO 2.4: constructor (addr, width, type)
  // TODO 2.4: addr(), width(), type()

 private:
  // TODO 2.4: static helper that builds the message
  // TODO 2.4: the three stored values
};

}  // namespace rvsim
```

`src/memory_fault.cpp` holds `to_string`, the constructor, and the message helper.

### The bounds check

One private helper in `Memory`, called at the top of both `read` and `write`. It takes the offset, the width, and the `AccessType` to report, and it:

1. asserts that the width is 1, 2, or 4 (this replaces the 2.2 assert);
2. throws `MemoryFault` unless the whole access fits: every byte from `offset` to `offset + width - 1` must exist, so `offset + width` must not exceed `size()`.

Check the whole access, not just its first byte: a 4-byte read at offset 13 of a 16-byte memory starts inside and ends outside.

Because `width` is a `std::size_t`, `offset + width` is computed in 64 bits and can't wrap. Done in 32 bits, offset 0xFFFFFFFE plus 4 wraps to 2 and passes the check.

### Check (T2.4)

`EXPECT_THROW(statement, MemoryFault)` checks that something throws. To check the fields, catch the fault yourself. A small function template keeps each test short: it takes a lambda, calls it inside `try`, catches `const MemoryFault&`, and returns the fault in a `std::optional<MemoryFault>`, empty if nothing threw. Then `ASSERT_TRUE` that it holds a fault and `EXPECT_EQ` each field.

Cases: a 4-byte read starting 1, 2, and 3 bytes before the end; a 1-byte write at offset `size()`; and the last valid access at each width, which must not throw.

### Gotchas

- **`[[nodiscard]]` inside `EXPECT_THROW`.** `EXPECT_THROW(mem.read(13, 4), MemoryFault)` fails to compile with `ignoring return value ... declared with attribute 'nodiscard'`, because the macro throws the result away. Write `EXPECT_THROW((void)mem.read(13, 4), MemoryFault)`. The same goes for `EXPECT_NO_THROW`, and for the bus's `read` and `fetch` if you mark them `[[nodiscard]]` as well.
- Catch exceptions by `const` reference. Catching by value makes a copy, and catching a derived exception by value as its base slices it.

---

## Part 5: Bus (2.5)

### The idea

The bus keeps a list of mappings, each a base address and the device that lives there. Every access runs the same three steps:

1. Find the mapping whose range holds the address.
2. Check that the whole access fits inside that device.
3. Forward it with `offset = addr - base`.

Any failure throws `MemoryFault` with the **bus** address and the access type of the entry point, so a failed `fetch` says `Fetch`. Write the three steps once, in a private helper (call it `lookup`), and have `fetch`, `read`, and `write` call it. The helper can return a small private struct that holds a `Device&` and the offset.

### Ownership

- The mapping list: a `std::vector` of a small private struct holding an `Addr` base and a `std::unique_ptr<Device>`.
- `map` takes the `std::unique_ptr<Device>` by value. The caller gives up ownership with `std::move(ptr)`, or passes `std::make_unique<Memory>(size)` directly: a `std::unique_ptr<Memory>` converts to a `std::unique_ptr<Device>` on its own.
- When the `Bus` is destroyed, the vector destroys the `unique_ptr`s, and they delete the devices through `Device*`. That is the moment the virtual destructor from 2.1 matters.
- Holding `unique_ptr`s makes the whole `Bus` move-only for free. Copying one doesn't compile, which is right: two buses can't own the same memory.

### Containment without overflow

The device holds `addr` when `addr >= base` and `addr - base < size()`. Subtract first. Never compute `base + size` in 32 bits: a device at 0xFFFFF000 covering 0x1000 bytes ends at 0x1_0000_0000, which wraps to 0 in an `Addr`.

The access then fits when `offset + width <= size()`. If it doesn't, throw. Don't go looking for another device, even one that starts right after: 2.5 says an access must fit inside one device.

### The overlap check in `map`

Treat ranges as half open: a device covers [base, base + size). Two ranges [a, a + n) and [b, b + m) overlap exactly when `a < b + m` and `b < a + n`. Compute the ends in `std::uint64_t`. Also reject a range whose end is past 2^32: 16 bytes at 0xFFFFFFF8 run off the top, while 16 bytes at 0xFFFFFFF0 end exactly at the top and are fine. Touching ranges, where one ends where the next begins, don't overlap.

Do every check before inserting, so a `map` that throws leaves the bus unchanged. Throw `std::invalid_argument` (`<stdexcept>`): a bad mapping is a mistake in setup code, not a guest program fault, so it isn't a `MemoryFault`.

### Why fetch is its own function

A fetch reaches the same memory as `read`, so it could have been `read(addr, 4)`. Keeping it separate pays off twice: a fetch fault is a different stop reason from a data fault (Lab 7), and in Lab 15 fetches go through the instruction cache while data goes through the data cache.

### Scaffold: `include/rvsim/bus.hpp`

```cpp
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "rvsim/device.hpp"
#include "rvsim/memory_fault.hpp"

namespace rvsim {

// TODO 2.6: the BusValue concept

class Bus {
 public:
  // TODO 2.5: map(base, device)
  // TODO 2.5: fetch(addr)
  // TODO 2.5: read(addr, width)
  // TODO 2.5: write(addr, width, value)

  // TODO 2.6: read<T>(addr) and write<T>(addr, value), defined here in the header

 private:
  // TODO 2.5: struct for one mapping: base and owned device
  // TODO 2.5: struct for a lookup result: device reference and offset
  // TODO 2.5: lookup(addr, width, type): returns a lookup result or throws
  // TODO 2.5: the list of mappings
};

}  // namespace rvsim
```

`src/bus.cpp` holds `map`, `fetch`, `read`, `write`, and `lookup`.

### Gotchas

- After `bus.map(base, std::move(ram))`, `ram` is null. To look inside a device from a test after mapping it, save a raw pointer first (`Memory* ram_ptr = ram.get();`), then move. The bus owns the device; the raw pointer only looks at it, and it stays valid as long as the bus lives.
- Assert the width (1, 2, or 4) in the bus too. The memory would catch it, but the bus is the entry point every later lab calls.

### Check (T2.5)

Map two small memories at different bases and check, through their raw pointers, that each address lands in the right device at the right offset. Then: an unmapped address throws; a read that starts inside a device and runs off its end throws, even with another device starting right after it; a failed `fetch` reports `AccessType::Fetch` and the bus address; an overlapping `map` throws `std::invalid_argument`, and a touching one doesn't.

---

## Part 6: Typed access (2.6)

### The idea

Often the width is known when you write the code: a halfword store always writes 2 bytes. Templates let the type carry the width: `bus.write(addr, static_cast<std::uint16_t>(v))` is a 2-byte write, with `sizeof(T)` supplying the width. The templates are a thin layer over the 2.5 functions, which call the virtual device functions. Templates on top, virtual dispatch underneath.

### Writing a concept

A concept is a named yes-or-no question about a type, answered at compile time. The syntax, on an unrelated example:

```cpp
template <typename T>
concept Small = sizeof(T) <= 2;

template <Small T>
void store(T value);  // store(std::uint16_t{1}) compiles; store(1.0) does not
```

`BusValue` should be true for exactly `std::uint8_t`, `std::uint16_t`, and `std::uint32_t`: three `std::same_as<T, ...>` checks from `<concepts>`, joined with `||`.

Why not the standard `std::unsigned_integral`? It also accepts `bool`, `char16_t`, and `std::uint64_t`, so `bus.write(addr, true)` would compile as a 1-byte write, and `bus.read<std::uint64_t>(addr)` would ask for 8 bytes.

### The two templates

- Put `template <BusValue T>` in front of each. They are members of `Bus` that overload the 2.5 functions with the same names, and both must be defined in the header: the compiler needs a template's body wherever it's used.
- `write<T>(addr, value)` forwards to `write(addr, sizeof(T), value)`. `T` is deduced from the argument, so callers rarely write it.
- `read<T>(addr)` forwards to `read(addr, sizeof(T))` and converts the `Word` to `T` with `static_cast`; that narrows, so `-Wconversion` insists on the cast. It's safe: only `sizeof(T)` bytes came back. `T` can't be deduced from a return type, so callers write `bus.read<std::uint16_t>(addr)`.
- `sizeof(T)` is a `std::size_t`, and so is the width parameter: no casts. That's the type rule from the top of this file paying off.

### Why these live on Bus and not on Device

Put a template `read<T>` in `Device`, and `Memory`'s `read(offset, width)` override **hides** it: a function declared in a derived class hides every base function with the same name, templates included. Then `memory.read<std::uint16_t>(0)` fails with a baffling `expected primary-expression before '>' token`. The fix would be `using Device::read;` and `using Device::write;` in every device. This is the "overriding versus hiding" question from Epic Duel's reflection. `Bus` has no base class, so nothing hides anything.

### Gotcha

`bus.write(addr, 0xFF)` doesn't compile: `0xFF` is an `int`, which isn't a `BusValue`, and GCC says `no matching function` with `constraints not satisfied`. That is the concept doing its job. Say the width: `bus.write(addr, std::uint8_t{0xFF})`.

### Check (T2.6): a typed test

A typed test runs one test body once for each type in a list. The pieces:

```cpp
template <typename T>
class BusTyped : public ::testing::Test {
 protected:
  // TODO: a Bus member, and a constructor that maps a small Memory
};

using BusValueTypes = ::testing::Types<std::uint8_t, std::uint16_t, std::uint32_t>;
TYPED_TEST_SUITE(BusTyped, BusValueTypes);

TYPED_TEST(BusTyped, RoundTrip) {
  // TODO: write a TypeParam value, read it back, compare
}
```

Inside the body, `TypeParam` is the current type, and fixture members are reached through `this->`. A good test value is `static_cast<TypeParam>(0x12345678)`: it gives 0x78, 0x5678, and 0x12345678, with every byte different, so a byte-order bug can't hide.

**Gotcha:** `this->bus.read<TypeParam>(addr)` fails with `expected 'template' keyword before dependent template name`. The fixture is a template, so the compiler can't tell that `read` is a template until it knows `T`, and you have to say so: `this->bus.template read<TypeParam>(addr)`. `write` needs nothing, because its `T` is deduced.

---

## Part 7: Loader (2.7)

### The idea

Put a block of bytes onto the bus, one byte at a time, starting at an address. Unit tests use it to place hand-assembled programs from Lab 7 on, and Lab 8's file loaders read a file into bytes and hand them to it.

### `std::span<const std::uint8_t>`

A span (`<span>`) is a view: a pointer and a length, owning nothing. A `std::span<const std::uint8_t>` parameter accepts a `std::vector<std::uint8_t>`, a `std::array<std::uint8_t, N>`, or a plain array without copying, so the loader doesn't care where the bytes live. Pass it by value; it's two words.

### Notes

- Byte `i` goes to `start + i`. `i` is a `std::size_t`, so the sum is 64 bits wide; convert it to `Addr` with `static_cast`. A typed write (`bus.write(addr, byte)`) deduces `std::uint8_t` from the byte: 2.6 paying off.
- A load that runs past the end of memory throws from the bus at the first missing byte, and the bytes before it stay written. That's fine for a loader: a program that doesn't fit won't run anyway.
- Optional: a load whose end passes 0xFFFFFFFF wraps around to low addresses when you convert to `Addr`. To rule that out, check `start + bytes.size()` against 2^32 once, before the loop.

### Scaffold: `include/rvsim/loader.hpp`

```cpp
#pragma once

#include <cstdint>
#include <span>

#include "rvsim/bus.hpp"

namespace rvsim {

// TODO 2.7: load_bytes(bus, start, bytes)
// Lab 8 adds load_binary and load_elf here.

}  // namespace rvsim
```

### Check (T2.7)

Load 13 05 50 00 at 0x80000000 and fetch: 0x00500513. Load the first four words of `first.S` (16 bytes) and fetch each one. Load four bytes starting 2 bytes before the end of memory: `MemoryFault`.

The 16 bytes, in load order: `13 05 50 00  93 05 70 00  33 06 b5 00  b3 86 a5 40` (`addi a0`, `addi a1`, `add a2`, `sub a3`).

---

## Part 8: Hex dump (2.8)

### The idea

A string that shows memory the way `xxd` or a debugger does. You'll reach for it when a program misbehaves and you want to see what the loader actually put where.

Each line: the address of the line's first byte in 8 hex digits, a colon, then each byte as a space plus two hex digits, then `\n`. Sixteen bytes per line; the last line can be shorter. Printing each byte as " xx" means there's never a trailing space.

### Notes

- Build the string in a `std::ostringstream` and return `.str()`. The formatting flags then live in that local stream and can't leak into `std::cout`.
- `std::hex` and `std::setfill` are sticky: they stay set until you change them. `std::setw` is not: it applies to the next output only, so set it before every field.
- Two loops: the outer one steps through the range 16 bytes at a time, and the inner one prints that line's bytes and stops at the end of the range.
- Read each byte through the bus with a 1-byte read. `hex_dump` takes a `Bus&`, not a `const Bus&`, because bus reads aren't `const` (Part 1).

### Gotcha

To a stream, a `std::uint8_t` is a character: printing a byte that holds 0x41 prints `A`, not `41`. Convert to `unsigned` before printing. `bus.read<std::uint8_t>` returns exactly such a byte.

### Scaffold: `include/rvsim/hex_dump.hpp`

```cpp
#pragma once

#include <cstddef>
#include <string>

#include "rvsim/bus.hpp"

namespace rvsim {

// TODO 2.8: hex_dump(bus, start, length)

}  // namespace rvsim
```

### Check (T2.8)

After loading the first four words of `first.S` at 0x80000000, `hex_dump(bus, 0x80000000, 16)` must equal the 2.8 string exactly, newline included. A 20-byte dump adds a second line, `80000010: 00 00 00 00`.

---

## Expected shape

When everything passes, these hold:

```
hex_dump(bus, 0x80000000, 20) after loading the first four words of first.S:
80000000: 13 05 50 00 93 05 70 00 33 06 b5 00 b3 86 a5 40
80000010: 00 00 00 00

what() for a 4-byte read at offset 13 of a 16-byte Memory:
memory fault: 4-byte read at 0x0000000d

what() for a 4-byte read at 0x80100000, through a Bus with 1 MiB at 0x80000000:
memory fault: 4-byte read at 0x80100000
```

Your message wording can differ; the fields can't.

---

## Progress checklist

**Device and Memory**
- [ ] 2.1 `Device`: virtual destructor; pure virtual `size()`, `read()`, `write()`
- [ ] 2.2 `Memory`: zeroed storage, `size()`, 1-byte `read` and `write`
- [ ] 2.3 2- and 4-byte access, little-endian, any alignment
- [ ] 2.4 `AccessType`, `MemoryFault`, bounds checks in `Memory`

**Bus and helpers**
- [ ] 2.5 `Bus`: `map`, `fetch`, `read`, `write`; faults carry bus addresses; overlaps rejected
- [ ] 2.6 `BusValue`, `read<T>`, `write<T>`
- [ ] 2.7 `load_bytes`
- [ ] 2.8 `hex_dump`

**Tests**
- [ ] T2.1 `static_assert`s on `Device`
- [ ] T2.2 new memory, 1-byte round trips
- [ ] T2.3 byte order, neighbors untouched, every width at both ends
- [ ] T2.4 faults at the end, with the right fields
- [ ] T2.5 routing, unmapped addresses, running off a device, fetch faults, overlaps
- [ ] T2.6 typed round trip for all three types
- [ ] T2.7 loader places bytes
- [ ] T2.8 hex dump string, exactly
- [ ] Everything passes in the sanitizer build

---

## Stretch work

1. **Prove the virtual destructor matters.** Remove `virtual` from `~Device()` and rebuild the debug build. It compiles: none of your warning flags notice, because the `delete` happens inside `std::unique_ptr`, in a system header. Run the bus tests: AddressSanitizer stops with `new-delete-type-mismatch`, since the object was allocated as a `Memory` and freed as a `Device`. Now add `-Wnon-virtual-dtor` to `rvsim_options` and watch the compiler catch the same mistake at build time. Keep the flag, and put `virtual` back.
2. **Watch name hiding happen.** Move the typed templates onto `Device` and try `memory.read<std::uint16_t>(0)`. Read the error, fix it with using-declarations in `Memory`, then decide which design you prefer.
3. **Non-virtual interface.** Make `Device::read` and `Device::write` public non-virtual functions that do the bounds check and then call private pure virtual `do_read` and `do_write`. Every device, including Lab 8's console, then gets bounds checking without writing it. `std::streambuf` is built this way.
4. **Typed `map`.** Make `map` a member template that takes a `std::unique_ptr<D>`, constrained with `std::derived_from<D, Device>`, and returns a `D&` to the device it just mapped. Tests no longer need the raw pointer trick.
5. **Faster lookup.** Replace the linear search with a `std::map` keyed by base address and use `upper_bound`. Every test should pass unchanged. Measuring speed waits for Extension E8.
6. **Straddling.** Map a second memory right after the first and do a 4-byte read that starts 2 bytes before the boundary. All four bytes exist, and it still faults. Decide whether that's what you want, and record why in `DESIGN.md`.

---

## Reflection

Once the tests pass, answer these. The first one is the lab's "done when" question.

- Why do instruction fetch and data access reach the same memory in your design? What would you change to model a Harvard machine, with separate instruction and data memories?
- Why does the bus translate addresses instead of each device knowing its own base?
- Why do reads zero-extend here, and why does sign extension wait for the load instruction?
- Who owns each device, and on which line of code is it destroyed?
- Why is a bad address an exception while a bad width is an `assert`?
