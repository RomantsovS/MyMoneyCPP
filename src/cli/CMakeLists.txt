# The command-line frontend, pure C++, desktop only (ADR-0010, ADR-0014).
# Everything except main() is a library, so tests can run real commands.

add_library(mymoney_cli_lib STATIC
  application.h
  arguments.cpp
  arguments.h
  command_table.cpp
  command_table.h
  context.h
  environment.cpp
  environment.h

  commands/catalog_commands.cpp
  commands/command_helpers.cpp
  commands/command_helpers.h
  commands/commands.h
  commands/operation_commands.cpp
  commands/report_commands.cpp
)
add_library(mymoney::cli_lib ALIAS mymoney_cli_lib)
target_include_directories(mymoney_cli_lib PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(mymoney_cli_lib PUBLIC mymoney::core mymoney::storage)
mymoney_target_defaults(mymoney_cli_lib)

add_executable(mymoney_cli main.cpp)
if(WIN32)
  # UTF-8 as the active code page: arguments arrive as UTF-8 (ADR-0014).
  target_sources(mymoney_cli PRIVATE mymoney-cli.manifest)
endif()
# Not "mymoney": Windows file names are case-insensitive, so it would collide
# with the app's MyMoney.exe in the same install folder (ADR-0011).
set_target_properties(mymoney_cli PROPERTIES OUTPUT_NAME mymoney-cli)
target_link_libraries(mymoney_cli PRIVATE mymoney::cli_lib)
mymoney_target_defaults(mymoney_cli)

# Installed next to the app; needs no Qt libraries.
include(GNUInstallDirs)
install(TARGETS mymoney_cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
