# ADR-0003: GoogleTest for unit tests

- **Status:** Accepted · 2026-10-13
- **Plan item:** D8

## Context

`core` is pure C++ (ADR-0002), so its tests do not need a Qt-specific framework.
GoogleTest is the industry standard and is familiar from work.

## Decision

- Unit tests use **GoogleTest** (with GoogleMock where useful).
- It is fetched by CMake `FetchContent`, pinned to a specific release tag.
- Tests are registered with CTest (`gtest_discover_tests`), so they run from Qt Creator,
  the command line, and CI.
- Tests run on desktop (Windows, Linux), not on Android.

## Consequences

- One third-party dependency, only for test builds.
- Qt-dependent code (`ui`) is not covered by these tests; Qt Quick Test may be added later.
