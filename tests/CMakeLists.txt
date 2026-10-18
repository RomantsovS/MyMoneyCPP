# Unit tests: one executable per layer (ADR-0011).

include(GoogleTest)

# --- core ---------------------------------------------------------------------
add_executable(mymoney_core_tests
  core/account_service_test.cpp
  core/amount_text_test.cpp
  core/balance_service_test.cpp
  core/category_service_test.cpp
  core/currency_code_test.cpp
  core/date_text_test.cpp
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

# --- storage ------------------------------------------------------------------
add_executable(mymoney_storage_tests
  storage/migrations_test.cpp
  storage/repository_contract_test.cpp
  storage/sqlite_wrapper_test.cpp
  storage/text_mapping_test.cpp
)
# The contract tests use the core fakes and test helpers.
target_include_directories(mymoney_storage_tests PRIVATE "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(mymoney_storage_tests PRIVATE mymoney::storage GTest::gmock_main)
mymoney_target_defaults(mymoney_storage_tests)
gtest_discover_tests(mymoney_storage_tests)

# --- cli ----------------------------------------------------------------------
if(MYMONEY_BUILD_CLI)
  add_executable(mymoney_cli_tests
    cli/arguments_test.cpp
    cli/cli_test.cpp
  )
  target_include_directories(mymoney_cli_tests PRIVATE "${PROJECT_SOURCE_DIR}/tests")
  target_link_libraries(mymoney_cli_tests PRIVATE mymoney::cli_lib GTest::gmock_main)
  mymoney_target_defaults(mymoney_cli_tests)
  gtest_discover_tests(mymoney_cli_tests)
endif()
