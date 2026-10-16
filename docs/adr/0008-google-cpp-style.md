# ADR-0008: Google C++ Style Guide

- **Status:** Accepted · 2026-10-13 · amended by [ADR-0011](0011-build-system-and-skeleton.md)
  (source files use `.cpp` instead of `.cc`)
- **Plan items:** D17, D16

## Decision

Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
as far as possible:

| Element | Rule | Example |
|---|---|---|
| Types, type aliases, enums | `PascalCase` | `Money`, `OperationType` |
| Functions | `PascalCase`; simple accessors may look like variables | `PostOperation()`, `amount()`, `set_amount()` |
| Variables, parameters | `snake_case` | `account_id` |
| Private/protected data members | `snake_case_` | `minor_units_` |
| Struct data members | `snake_case` | `account_id` |
| Constants, enumerators | `kPascalCase` | `kMaxCategoryDepth`, `kExpense` |
| Namespaces | `snake_case` | `mymoney::core` |
| Macros | `UPPER_CASE` | `MYMONEY_CORE_MONEY_H_` |
| Files | `snake_case.h` / `snake_case.cc` | `money.h`, `money.cc` |

- Include guards: `#define` guards `MYMONEY_<PATH>_<FILE>_H_` (not `#pragma once`).
- **No exceptions** (Google rule). Errors are returned as `std::expected<T, Error>`.
  Programming errors (broken invariants) are checked with assertions. *This also decides D16.*
- Formatting: `.clang-format` with `BasedOnStyle: Google` (80 columns, 2-space indent).
- Naming and other rules are checked by `.clang-tidy` (`readability-identifier-naming` +
  selected checks). Both files are written in M0.

## Deliberate deviations

1. **C++23** is allowed (ADR-0002), although the guide targets an older standard.
2. **Names visible from QML** use Qt/QML conventions: `camelCase` properties, signals and
   invokable methods — so that QML handlers read `onTotalBalanceChanged`.
3. **Overrides of Qt virtual functions** keep Qt's names (`rowCount`, `data`, `roleNames`).

## Consequences

- One consistent style in `core`, `storage` and the CLI; a thin Qt-style surface in `ui`.
- Tooling enforces the style, so code review can focus on design.
