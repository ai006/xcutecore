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

//Test T2.3 (only testing writes and reads of a single byte)
TEST(TestingMemory, TestReadWrite1) {
    std::size_t size = 20;
    rvsim::Device* memory = new rvsim::Memory(size);
    EXPECT_EQ(memory->size(), size);


    //write to memory & read from memory
    Word data = 0x12345678;
    memory->write(0, 4, data);
    EXPECT_EQ(memory->read(0, 4), data);

    //try to write more than a byte
    data = 0x3456;
    EXPECT_EQ(memory->read(1, 2), data);
    data = 0x12;
    EXPECT_EQ(memory->read(3, 1), data);

    data = 0xABCD1234;
    memory->write(8, 2, data);
    EXPECT_EQ(memory->read(8, 2), 0x1234);

    delete memory;
}

//Test T2.4: a 4-byte read that starts 1, 2, or 3 bytes before the end throws
TEST(TestingMemory, TestReadPastEnd) {
    std::size_t size = 16;
    rvsim::Device* memory = new rvsim::Memory(size);
 
    EXPECT_THROW((void)memory->read(13, 4), rvsim::MemoryFault); //3 bytes before the end
    EXPECT_THROW((void)memory->read(14, 4), rvsim::MemoryFault); //2 bytes before the end
    EXPECT_THROW((void)memory->read(15, 4), rvsim::MemoryFault); //1 byte before the end
 
    //smaller reads past the end throw too
    EXPECT_THROW((void)memory->read(15, 2), rvsim::MemoryFault);
    EXPECT_THROW((void)memory->read(16, 1), rvsim::MemoryFault);
 
    //writes past the end throw too
    EXPECT_THROW(memory->write(13, 4, 0xAABBCCDD), rvsim::MemoryFault);
    EXPECT_THROW(memory->write(16, 1, 0xAA), rvsim::MemoryFault);
 
    delete memory;
}

//Test T2.4: the fault carries the right offset, width, and access type
TEST(TestingMemory, TestFaultFields) {
    std::size_t size = 16;
    rvsim::Device* memory = new rvsim::Memory(size);
 
    //a read that crosses the end
    try {
        (void)memory->read(13, 4);
        FAIL() << "read did not throw";
    } catch (const rvsim::MemoryFault& fault) {
        EXPECT_EQ(fault.getAddr(), 13);
        EXPECT_EQ(fault.getWidth(), 4);
        EXPECT_EQ(fault.getAccessType(), rvsim::AccessType::Read);
    }
 
    //a write that crosses the end
    try {
        memory->write(15, 2, 0xABCD);
        FAIL() << "write did not throw";
    } catch (const rvsim::MemoryFault& fault) {
        EXPECT_EQ(fault.getAddr(), 15);
        EXPECT_EQ(fault.getWidth(), 2);
        EXPECT_EQ(fault.getAccessType(), rvsim::AccessType::Write);
    }
 
    delete memory;
}
 
//Test T2.4: the last valid access at each width does not throw
TEST(TestingMemory, TestLastValidAccess) {
    std::size_t size = 16;
    rvsim::Device* memory = new rvsim::Memory(size);
 
    EXPECT_NO_THROW(memory->write(15, 1, 0xAA));       //last byte
    EXPECT_NO_THROW(memory->write(14, 2, 0xAABB));     //last 2 bytes
    EXPECT_NO_THROW(memory->write(12, 4, 0xAABBCCDD)); //last 4 bytes
 
    EXPECT_NO_THROW((void)memory->read(15, 1));
    EXPECT_NO_THROW((void)memory->read(14, 2));
    EXPECT_NO_THROW((void)memory->read(12, 4));
 
    delete memory;
}
