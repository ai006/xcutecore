#include <gtest/gtest.h>

#include "rvsim/bits.hpp"

// tutorial on Google Test(GTest)
//https://helpmetest.com/blog/gtest-tutorial-cpp-unit-testing/

TEST(TestingBits, ExtractSubstring1) {
  EXPECT_EQ(rvsim::bits(0x00B50633, 6, 0), 0x33);
}

TEST(TestingBits, ExtractSubstring2) {
  EXPECT_EQ(rvsim::bits(0x00B50633, 11, 7), 12);
}

TEST(TestingBits, ExtractSubstring3) {
  EXPECT_EQ(rvsim::bits(0x00B50633, 19, 15), 10);
}

TEST(TestingBits, ExtractSubstring4) {
  EXPECT_EQ(rvsim::bits(0x00B50633, 24, 20), 11);
}

TEST(TestingBits, ExtractSubstring5) {
  EXPECT_EQ(rvsim::bits(0xDEADBEEF, 31, 28), 0xD);
}

TEST(TestingBits, ExtractSubstring6) {
  EXPECT_EQ(rvsim::bits(0xDEADBEEF, 31, 0), 0xDEADBEEF);
}

TEST(TestingBits, CheckSingleBit1) {
  EXPECT_TRUE(rvsim::bit(0x40A586B3, 30));
}

TEST(TestingBits, CheckSingleBit2) {
  EXPECT_FALSE(rvsim::bit(0x00B50633, 30));
}

TEST(TestingBits, SignExtend1) {
  EXPECT_EQ(rvsim::sign_extend(0x7FF, 12), 0x000007FF);
}

TEST(TestingBits, SignExtend2) {
  EXPECT_EQ(rvsim::sign_extend(0x800, 12), 0xFFFFF800);
}

TEST(TestingBits, SignExtend3) {
  EXPECT_EQ(rvsim::sign_extend(0xFFF, 12), 0xFFFFFFFF);
}

TEST(TestingBits, SignExtend4) {
  EXPECT_EQ(rvsim::sign_extend(0xABC00FFF, 12), 0xFFFFFFFF);
}

TEST(TestingBits, SignExtend5) {
  EXPECT_EQ(rvsim::sign_extend(0x1, 1), 0xFFFFFFFF);
}

TEST(TestingBits, SignExtend6) {
  EXPECT_EQ(rvsim::sign_extend(0x12345678, 32), 0x12345678);
}

TEST(TestingBits, toBinaryString) {
  std::string str = "0101 0000 0110 0011 1100 1110 1101 1010";
  EXPECT_EQ(rvsim::to_binary_string(0x5063CEDA), str);
}
