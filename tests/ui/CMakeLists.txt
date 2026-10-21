# mymoney_ui_tests: the Qt models, editors and amount formatting (ADR-0016).
# The only test executable that needs Qt; added from the Qt part of the
# top-level CMakeLists.txt.

include(GoogleTest)

add_executable(mymoney_ui_tests
  account_list_model_test.cpp
  editors_test.cpp
  money_format_test.cpp
  qt_print.h
  ui_test_base.h
  ui_test_main.cpp
)
# Test helpers are included from tests/: "core/test_util.h", "ui/qt_print.h".
target_include_directories(mymoney_ui_tests PRIVATE "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(mymoney_ui_tests PRIVATE
  mymoney_ui
  mymoney::storage
  Qt6::Test
  GTest::gtest  # our own main(): ui_test_main.cpp
)
mymoney_target_defaults(mymoney_ui_tests)

# Test discovery runs the executable right after it is linked. On Windows it
# only starts if the Qt DLLs are next to it (they are not on PATH).
# qt_standard_project_setup() puts every executable into the top build folder,
# next to MyMoney.exe - copied DLLs there make the app load Qt from the build
# folder, where it finds no platform plugin. So the tests get their own folder.
if(WIN32)
  set_target_properties(mymoney_ui_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  add_custom_command(TARGET mymoney_ui_tests POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      $<TARGET_RUNTIME_DLLS:mymoney_ui_tests> $<TARGET_FILE_DIR:mymoney_ui_tests>
    COMMAND_EXPAND_LISTS)
endif()
gtest_discover_tests(mymoney_ui_tests)
