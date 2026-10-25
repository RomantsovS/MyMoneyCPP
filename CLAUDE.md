# CLAUDE.md

Guidance for Claude Code sessions in this repository. Keep it short and current.

## Where things are

- `docs/PROJECT_PLAN.md`: goals, architecture, milestones (status of each), open decisions.
  **Read it at the start of a session.**
- `docs/adr/NNNN-*.md`: why each decision was made. Accepted ADRs are not rewritten; a change
  gets a new ADR or a dated amendment (ADR-0001).
- `README.md`: build instructions, CLI usage, project status.

## How we work

Design first, then code (PROJECT_PLAN section 0): discuss options → the user decides → record
an ADR → implement on a branch → explain → the user reviews locally → commit and merge.
Never write feature code before its design is agreed.

Git:
- One branch per milestone (`m3-cli`, `m3b-category-groups`). Commit the ADR / plan update
  first; leave the implementation **uncommitted** until the user has reviewed it.
- No GitHub pull requests. When the user says "commit and push": one commit, then
  `git merge --ff-only` into `master`, push, and delete the branch.
- Before rewriting the plan or other docs, commit the previous version first.
- PowerShell 5.1 breaks quoted multi-line messages: write the message to a file and use
  `git commit -F <file>`.

## Architecture rules

- `src/core`: domain logic, **pure C++23, no Qt, no I/O**. Repository interfaces live here.
- `src/storage`: SQLite behind the core interfaces, pure C++23. `sqlite3.h` is included only
  in `storage/sqlite/*.cpp`.
- `src/cli`: `mymoney_cli_lib` (commands return text) + `main.cpp` (the only place that prints).
- `src/ui`, `src/app`: the Qt Quick app, the only code that uses Qt.
- Targets are added in this order in the top-level `CMakeLists.txt`: the pure C++ part before
  `qt_standard_project_setup()`, so AUTOMOC never touches it.

## Conventions

- Google C++ style (ADR-0008) with `.cpp`/`.h` files (ADR-0011). Includes are written from
  `src/` (`#include "core/money.h"`); header guards `MYMONEY_<PATH>_<FILE>_H_`.
- No exceptions: errors are `core::Result<T>` / `core::Status` (`std::expected`). Broken
  invariants: `core::Check()` (active in Release too).
- Money is `int64` minor units + currency, never floating point.
- Every business rule has a test. Repository behaviour is covered by the **contract tests**
  (`tests/storage/repository_contract_test.cpp`), which run against both the in-memory fakes
  (`tests/core/fakes/`) and SQLite. The fakes must mirror the database constraints.
- New schema changes: a new numbered file in `src/storage/migrations/`, registered in
  `src/storage/CMakeLists.txt`. Never edit a migration that has been committed.
- **clang-tidy must report 0 findings** (findings are errors). Suppress only with
  `NOLINT(<check>)` plus the reason in a comment. Run clang-format on every changed file.

## Build and test (this machine)

Windows tools: Qt 6.12.0 in `C:\Qt`, Qt Creator 20, VS 2026 (MSVC), Qt's CMake 3.30 and Ninja
in `C:\Qt\Tools`. Command-line builds need the MSVC environment:

```bat
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;%PATH%
cmake --preset windows-noui-debug && cmake --build --preset windows-noui-debug && ctest --preset windows-noui-debug
```

- `windows-noui-debug`: core, storage, CLI and all tests, without Qt (fastest check).
- `local-windows-debug` / `local-windows-release`: with Qt; defined in the git-ignored
  `CMakeUserPresets.json` (Qt path). `--target install` creates a runnable `dist/<preset>/`.
- From the PowerShell tool, run the lines above through one `cmd /c "... && ..."`, with
  `C:\Program Files (x86)\Microsoft Visual Studio\Installer` prepended to `PATH`: without
  it `vcvarsall.bat` cannot find `vswhere.exe`, and the failed configure then caches empty
  compiler flags (no `/EHsc`) - fix that with `cmake --preset <name> --fresh`.

**Linux** (WSL distro `Ubuntu-26.04`, GCC 15, CMake 4.2):
`wsl -d Ubuntu-26.04 -- bash -lc "cd /mnt/c/Users/roman/source/repos/MyMoney && cmake --preset linux-noui-debug && cmake --build --preset linux-noui-debug && ctest --preset linux-noui-debug"`.
Start `wsl` with a timeout (`Start-Process ... WaitForExit`): a distro waiting for input
blocks every command.

**Android** builds normally run in Qt Creator (kits `Qt 6.12.0 for Android arm64-v8a` for
the Pixel 6, `x86_64` for the emulator AVD `Emulator_Pixel6`). From the terminal, reuse Qt Creator's
build folder; it uses the NMake/JOM generator:

```bat
set PATH=C:\Qt\Tools\QtCreator\bin\jom;C:\Qt\Tools\CMake_64\bin;%PATH%
cmake -S . -B build\Qt_6_12_0_for_Android_arm64_v8a_Debug
cmake --build build\Qt_6_12_0_for_Android_arm64_v8a_Debug --target mymoney_app
cmake --build build\Qt_6_12_0_for_Android_arm64_v8a_Debug --target apk
```

`mymoney_app` builds only the native library; the `apk` target packages it
(`src\app\android-build-mymoney_app\mymoney_app.apk`). Gradle leaves a daemon running that
keeps output pipes open: do not wait on piped Gradle output; check for the `.apk` file instead.
The daemon also keeps a redirected log file open: give every build a new log file name, and
always compare the `.apk` timestamp - an old log can look like a successful build.

**Static analysis** (Qt Creator's LLVM 22), using the compile commands of a build folder:

```bat
C:\Qt\Tools\QtCreator\bin\clang\bin\clang-tidy.exe -p build\windows-noui-debug --quiet <file.cpp>
C:\Qt\Tools\QtCreator\bin\clang\bin\clang-format.exe -i <files>
cmake --build --preset local-windows-debug --target mymoney_ui_qmllint
```

UI files need a Qt build folder for clang-tidy (`-p build\windows-debug`). qmllint must
report 0 warnings too (ADR-0016).

## Gotchas

- PowerShell treats `12,50` as an array: quote such amounts (`"12,50"`) when running the CLI.
- `mymoney-cli.exe` uses a UTF-8 manifest; `[Console]::OutputEncoding = [Text.Encoding]::UTF8`
  makes PowerShell show its Cyrillic output correctly.
- Line endings are LF everywhere (`.gitattributes`).
