# MyMoney

A personal finance tracker: income, expenses and transfers between accounts in several
currencies, balances and reports. Written in C++23 with a Qt Quick UI for **Android**
(main use) and a command-line interface for PC.

This is a personal learning project. Design comes first — see
[`docs/PROJECT_PLAN.md`](docs/PROJECT_PLAN.md) and the decision records in
[`docs/adr/`](docs/adr/).

**Status:** M1 (domain core) done — accounts, categories, operations and balances with all
ledger rules, in pure C++23 with unit tests. Next: M2, storage with SQLite.

## Project layout

| Path | Target | What |
|---|---|---|
| `src/core/` | `mymoney_core` | domain logic, pure C++23 (no Qt) |
| `src/cli/` | `mymoney_cli` → `mymoney-cli` | command-line interface, pure C++23, desktop only |
| `src/ui/` | `mymoney_ui` | QML module `MyMoney.UI` |
| `src/app/` | `mymoney_app` → `MyMoney` | Qt Quick application (Android, Windows, Linux) |
| `tests/` | `mymoney_core_tests` | GoogleTest unit tests, desktop only |
| `cmake/` | — | compiler options, dependencies |

## Requirements

Versions used during development:

| Tool | Version |
|---|---|
| Qt | 6.12.0 (MSVC 2022 64-bit, Android arm64-v8a and x86_64) |
| Qt Creator | 20.0.2 |
| CMake / Ninja | 3.30 or newer / 1.12 (shipped with the Qt installer) |
| Windows compiler | MSVC from Visual Studio 2026 |
| Linux compiler | GCC 15 (Ubuntu 26.04) |
| Android | JDK 17, SDK platform 37, NDK 27.2.12479018 |

GoogleTest 1.18.0 is downloaded automatically by CMake.

## Build

### Qt Creator (desktop and Android)

Open `CMakeLists.txt` in Qt Creator and select the kits:

- *Desktop Qt 6.12.0 MSVC2022 64bit* — daily development, tests, debugging;
- *Qt 6.12.0 for Android x86_64* — the emulator;
- *Qt 6.12.0 for Android arm64-v8a* — a phone.

Android builds are done only through Qt Creator kits.

### Command line (Windows)

Use **Developer PowerShell for VS 2026** (Start menu): it provides the MSVC compiler
environment together with CMake and Ninja (Visual Studio's own copies). A normal PowerShell
does not find `cmake` or `cl`.

Without Qt (core, CLI, tests):

```
cmake --preset windows-noui-debug
cmake --build --preset windows-noui-debug
ctest --preset windows-noui-debug
```

With Qt, add a local, git-ignored `CMakeUserPresets.json` that tells CMake where Qt is:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "local-windows-debug",
      "inherits": "windows-debug",
      "binaryDir": "${sourceDir}/build/windows-debug",
      "cacheVariables": { "CMAKE_PREFIX_PATH": "C:/Qt/6.12.0/msvc2022_64" }
    },
    {
      "name": "local-windows-release",
      "inherits": "windows-release",
      "binaryDir": "${sourceDir}/build/windows-release",
      "installDir": "${sourceDir}/dist/windows-release",
      "cacheVariables": { "CMAKE_PREFIX_PATH": "C:/Qt/6.12.0/msvc2022_64" }
    }
  ],
  "buildPresets": [
    { "name": "local-windows-debug", "configurePreset": "local-windows-debug" },
    { "name": "local-windows-release", "configurePreset": "local-windows-release" }
  ],
  "testPresets": [{ "name": "local-windows-debug", "configurePreset": "local-windows-debug" }]
}
```

```
cmake --preset local-windows-debug
cmake --build --preset local-windows-debug
ctest --preset local-windows-debug
```

### A runnable copy (no Qt on `PATH` needed)

The `install` step creates a self-contained folder `dist/<preset>/` with `MyMoney.exe`,
`mymoney-cli.exe` and every Qt library, plugin and QML module they need:

```
cmake --build --preset local-windows-release --target install
dist\windows-release\bin\MyMoney.exe
```

The folder name comes from `installDir` in the local preset above.

### Command line (Linux)

```
cmake --preset linux-noui-debug
cmake --build --preset linux-noui-debug
ctest --preset linux-noui-debug
```

## Code style

[Google C++ Style](https://google.github.io/styleguide/cppguide.html) with `.cpp` files
([ADR-0008](docs/adr/0008-google-cpp-style.md), [ADR-0011](docs/adr/0011-build-system-and-skeleton.md)).
`.clang-format` and `.clang-tidy` define the rules; all clang-tidy findings are errors.
