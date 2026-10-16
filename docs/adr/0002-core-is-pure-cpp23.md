# ADR-0002: The core layer is pure standard C++23

- **Status:** Accepted · 2026-10-13 · amended 2026-10-16 (Linux toolchain, see the end)
- **Plan items:** D5, D9

## Context

The app uses Qt for the UI. The domain logic (money, accounts, categories, operations,
business rules) does not need Qt. One goal of the project is to learn modern standard C++.
The wish is to use "as little Qt as possible".

## Decision

- `mymoney_core` uses **only the C++23 standard library** — no Qt headers.
- Language standard for the whole project: **C++23** (`CMAKE_CXX_STANDARD 23`, no extensions).
- Use only C++23 features that work on all three toolchains:
  MSVC (VS 2026), Android NDK 27 clang 18 / libc++, and GCC on Linux.
- Text in `core` is `std::string` with **UTF-8** content. MSVC builds with `/utf-8`.
- Dates in `core` are `std::chrono::year_month_day`.
- Conversion to Qt types (`QString`, `QDate`) happens only at the border, in the `ui` layer.

## Consequences

- `core` can be built and unit-tested without Qt — fast, and runs anywhere.
- Some conversion code in `ui` (`std::string` ↔ `QString`, `year_month_day` ↔ `QDate`).
- GCC 13 (Ubuntu 24.04 default) lacks `std::print` and `std::ranges::to`.
  The Linux build will use **GCC 14** (`gcc-14` package in Ubuntu 24.04) instead.

## Amendment · 2026-10-16 — Linux toolchain

WSL now runs **Ubuntu 26.04 LTS** (replacing 24.04) with **GCC 15** as the Linux compiler.
GCC 15's libstdc++ provides `std::print` and `std::ranges::to`, so the GCC 14 workaround
above is no longer needed. The C++23 rule is unchanged: use only features that work on all
three toolchains (MSVC, NDK clang/libc++, GCC).
