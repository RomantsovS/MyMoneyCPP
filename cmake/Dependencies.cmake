# Third-party dependencies, pinned to exact versions (ADR-0011, ADR-0013).

include(FetchContent)

# --- SQLite (all platforms) ---------------------------------------------------
# The amalgamation: the whole database engine in one C file. It has no
# CMakeLists.txt, so FetchContent only downloads it and the library target is
# defined below. The hash is the SHA3-256 published on sqlite.org/download.html.
FetchContent_Declare(sqlite
  URL https://sqlite.org/2026/sqlite-amalgamation-3540100.zip
  URL_HASH SHA3_256=92281c86e2957f9b9305890f9ac04204caa7b677cc868dc326d942689df28e30
  DOWNLOAD_EXTRACT_TIMESTAMP ON)
FetchContent_MakeAvailable(sqlite)

find_package(Threads REQUIRED)

# Third-party code: built without our warning flags (no mymoney_target_defaults).
add_library(mymoney_sqlite3 STATIC "${sqlite_SOURCE_DIR}/sqlite3.c")
add_library(SQLite::SQLite3 ALIAS mymoney_sqlite3)
# SYSTEM: our strict warnings do not apply to sqlite3.h either.
target_include_directories(mymoney_sqlite3 SYSTEM PUBLIC "${sqlite_SOURCE_DIR}")
target_compile_definitions(mymoney_sqlite3 PRIVATE
  SQLITE_DQS=0                  # "text" in double quotes is an error, not a string
  SQLITE_DEFAULT_FOREIGN_KEYS=1 # foreign keys are always enforced
  SQLITE_OMIT_LOAD_EXTENSION    # no loading of native code from SQL
  SQLITE_DEFAULT_MEMSTATUS=0    # faster: no memory usage statistics
  SQLITE_OMIT_DEPRECATED
  SQLITE_OMIT_SHARED_CACHE
)
target_link_libraries(mymoney_sqlite3 PRIVATE Threads::Threads)

# --- GoogleTest (desktop tests only) ------------------------------------------
if(MYMONEY_BUILD_TESTS)
  # Link GoogleTest against the same (DLL) C runtime as our code on MSVC;
  # otherwise the linker reports a runtime library mismatch (LNK2038).
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
    URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    # Treat GoogleTest headers as system headers: our strict warnings do not
    # apply to them.
    SYSTEM)
  FetchContent_MakeAvailable(googletest)
endif()
