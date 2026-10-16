# Unit tests: one executable per layer (ADR-0011).

include(GoogleTest)

add_executable(mymoney_core_tests
  core/version_test.cpp
)
target_link_libraries(mymoney_core_tests PRIVATE mymoney::core GTest::gtest_main)
mymoney_target_defaults(mymoney_core_tests)

gtest_discover_tests(mymoney_core_tests)
