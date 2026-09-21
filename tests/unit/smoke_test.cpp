#include <gtest/gtest.h>

#include "rvsim/version.hpp"

TEST(Smoke, VersionIsNotEmpty) {
  EXPECT_FALSE(rvsim::version().empty());
}
