# MyMoney

A personal finance tracker: income, expenses and transfers between accounts in several
currencies, balances and reports. Written in C++23 with a Qt Quick UI for **Android**
(main use) and a command-line interface for PC.

This is a personal learning project. Design comes first — see
[`docs/PROJECT_PLAN.md`](docs/PROJECT_PLAN.md) and the decision records in
[`docs/adr/`](docs/adr/).

**Status:** M3 (command-line interface) done — all data can be entered and viewed with
mymoney-cli; 124 unit tests on Windows and Linux. Next: category groups, then the Qt UI (M4).

## Project layout

| Path | Target | What |
|---|---|---|
| `src/core/` | `mymoney_core` | domain logic, pure C++23 (no Qt) |
| `src/storage/` | `mymoney_storage` | SQLite storage, schema migrations, pure C++23 |
| `src/cli/` | `mymoney_cli_lib`, `mymoney_cli` → `mymoney-cli` | command-line interface, pure C++23, desktop only |
| `src/ui/` | `mymoney_ui` | QML module `MyMoney.UI` |
| `src/app/` | `mymoney_app` → `MyMoney` | Qt Quick application (Android, Windows, Linux) |
| `tests/` | `mymoney_{core,storage,cli}_tests` | GoogleTest unit tests, desktop only |
| `cmake/` | — | compiler options, dependencies, migration embedding |

## Using the CLI

A complete session, starting from an empty database:

```
mymoney-cli currency add EUR --digits 2
mymoney-cli currency add USD --digits 2

mymoney-cli account add "Card EUR" --currency EUR
mymoney-cli account add "Cash EUR" --currency EUR
mymoney-cli account add "Cash USD" --currency USD

mymoney-cli category add Salary --kind income
mymoney-cli category add Food --kind expense
mymoney-cli category add Restaurants --kind expense --parent Food

mymoney-cli income "Card EUR" Salary 3000 --date 2026-10-01
mymoney-cli transfer "Card EUR" "Cash EUR" 200 --date 2026-10-02
mymoney-cli transfer "Card EUR" "Cash USD" 100 --amount-in 108.50 --date 2026-10-03
mymoney-cli expense "Cash EUR" Restaurants 12.50 --comment lunch --date 2026-10-18

mymoney-cli journal --from 2026-10-01 --to 2026-10-31
mymoney-cli balance
```

`balance` then shows:

```
Card EUR   2700.00 EUR
Cash EUR    187.50 EUR
Cash USD    108.50 USD
Total EUR  2887.50 EUR
Total USD   108.50 USD
```

`mymoney-cli --help` lists all commands. Accounts and categories are referred to by name
(names are unique). The database is `%LOCALAPPDATA%\MyMoney\mymoney.sqlite` on Windows and
`~/.local/share/MyMoney/mymoney.sqlite` on Linux; `--db FILE` uses another one.

**PowerShell:** quote amounts written with a comma — `"12,50"`. Unquoted, PowerShell
treats the comma as its array operator and passes `12` and `50` as two arguments.

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
