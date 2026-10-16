# mymoney_core: domain logic in pure C++23, no Qt (ADR-0002).

configure_file(version.cpp.in version.cpp @ONLY)

add_library(mymoney_core STATIC
  version.h
  "${CMAKE_CURRENT_BINARY_DIR}/version.cpp"
)
add_library(mymoney::core ALIAS mymoney_core)

# Includes are written from src/: #include "core/version.h".
target_include_directories(mymoney_core PUBLIC "${PROJECT_SOURCE_DIR}/src")

mymoney_target_defaults(mymoney_core)
