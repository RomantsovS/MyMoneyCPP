# Third-party dependencies, pinned to exact versions (ADR-0011).

include(FetchContent)

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
