find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(unit_tests
  unit/smoke_test.cpp
)
target_link_libraries(unit_tests PRIVATE rvcore rvsim_options GTest::gtest_main)
gtest_discover_tests(unit_tests)
