# Unit tests: one executable per layer (ADR-0011).

include(GoogleTest)

add_executable(mymoney_core_tests
  core/account_service_test.cpp
  core/balance_service_test.cpp
  core/category_service_test.cpp
  core/currency_code_test.cpp
  core/money_test.cpp
  core/operation_draft_test.cpp
  core/operation_service_test.cpp
  core/version_test.cpp

  core/fakes/in_memory_repositories.h
  core/services_test_base.h
  core/test_util.h
)
# Test helpers are included from tests/: #include "core/test_util.h".
target_include_directories(mymoney_core_tests PRIVATE "${PROJECT_SOURCE_DIR}/tests")
# gmock_main provides main() and includes GoogleTest.
target_link_libraries(mymoney_core_tests PRIVATE mymoney::core GTest::gmock_main)
mymoney_target_defaults(mymoney_core_tests)

gtest_discover_tests(mymoney_core_tests)
