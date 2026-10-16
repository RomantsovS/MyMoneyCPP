# mymoney_cli: command-line frontend in pure C++, desktop only (ADR-0010).

add_executable(mymoney_cli main.cpp)
# Not "mymoney": Windows file names are case-insensitive, so it would collide
# with the app's MyMoney.exe in the same install folder (ADR-0011).
set_target_properties(mymoney_cli PROPERTIES OUTPUT_NAME mymoney-cli)
target_link_libraries(mymoney_cli PRIVATE mymoney::core)

mymoney_target_defaults(mymoney_cli)

# Installed next to the app; needs no Qt libraries.
include(GNUInstallDirs)
install(TARGETS mymoney_cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
