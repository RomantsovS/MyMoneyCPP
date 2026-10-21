cmake_minimum_required(VERSION 3.30)

# The version here is the single source of truth: it is compiled into core
# (core/version.cpp.in) and used for the Android version name and code.
project(MyMoney
  VERSION 0.1.0
  DESCRIPTION "Personal finance tracker"
  HOMEPAGE_URL "https://github.com/RomantsovS/MyMoneyCPP"
  # C only for the SQLite amalgamation (sqlite3.c); all our code is C++.
  LANGUAGES C CXX)

# The CLI and the tests are desktop tools; the Qt Quick app runs everywhere.
if(ANDROID)
  set(mymoney_is_desktop OFF)
else()
  set(mymoney_is_desktop ON)
endif()

option(MYMONEY_BUILD_UI "Build the Qt Quick application (needs Qt)" ON)
option(MYMONEY_BUILD_CLI "Build the command-line interface" ${mymoney_is_desktop})
option(MYMONEY_BUILD_TESTS "Build the unit tests" ${mymoney_is_desktop})

# We do not use C++20 modules; skip the per-file module dependency scan that
# CMake otherwise runs for C++20 and newer.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(CompilerOptions)
include(Dependencies)

# --- Pure C++ part -----------------------------------------------------------
# Added before Qt is set up: qt_standard_project_setup() turns on AUTOMOC for
# every target created after it in this directory scope, and these targets must
# not depend on Qt (ADR-0002).
add_subdirectory(src/core)
add_subdirectory(src/storage)

if(MYMONEY_BUILD_CLI)
  add_subdirectory(src/cli)
endif()

if(MYMONEY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# --- Qt part -----------------------------------------------------------------
if(MYMONEY_BUILD_UI)
  find_package(Qt6 6.12 REQUIRED COMPONENTS Gui Qml Quick)
  if(MYMONEY_BUILD_TESTS)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Test)  # mymoney_ui_tests
  endif()
  qt_standard_project_setup(REQUIRES 6.12)

  # All QML modules go to build/<preset>/qml/<URI path>, e.g. qml/MyMoney/UI.
  # QML tooling (qmllint, the QML language server) finds modules by this layout.
  set(QT_QML_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/qml")

  add_subdirectory(src/ui)
  add_subdirectory(src/app)

  if(MYMONEY_BUILD_TESTS)
    add_subdirectory(tests/ui)
  endif()
endif()
