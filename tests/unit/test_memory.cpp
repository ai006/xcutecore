#include <gtest/gtest.h>
#include <type_traits>

#include "rvsim/device.hpp"
#include "rvsim/memory.hpp"

//check if Device is an abstract class
static_assert(std::is_abstract_v<rvsim::Device> == true);
//check if Device has a virtual destructor
static_assert(std::has_virtual_destructor_v<rvsim::Device> == true);

//Test T2.2 (only testing writes and reads of a single byte)
TEST(TestingMemory, TestReadWrite) {
    std::size_t size = 10;
    rvsim::Device* memory = new rvsim::Memory(size);
    EXPECT_EQ(memory->size(), size);

    //check that memory has been initialized with zereos
    for(Addr offset = 0; offset < memory->size(); offset++){
        EXPECT_EQ(memory->read(offset, 1), 0x0);
    }

    //write to memory & read from memory
    Word data = 0xAB;
    memory->write(3, 1, data);
    EXPECT_EQ(memory->read(3, 1), data);

    //try to write more than a byte
    data = 0x1FF;
    memory->write(4, 1, data);
    EXPECT_EQ(memory->read(4, 1), 0xFF);
    EXPECT_EQ(memory->read(5, 1), 0x00);

    delete memory;
}