# ADR-0011: Build system and project skeleton (M0)

- **Status:** Accepted · 2026-10-16
- **Plan items:** D24, M0
- **Amends:** ADR-0008 (source file extension `.cpp` instead of `.cc`)

## Context

M0 sets up the build for four platforms (Windows, Linux, Android arm64 and x86_64) and
two frontends (Qt Quick app, CLI), with tests, formatting and static analysis — before any
feature code exists. These choices are hard to change later, so they are recorded here.

## Decision

1. **Scope of M0:** `mymoney_core` (only the generated version), `mymoney_cli`,
   `mymoney_core_tests`, `mymoney_ui` (QML module), `mymoney_app`. No `storage` until M2 —
   an empty target would compile nothing.
2. **CMake:** Qt's CMake 3.30.5 on Windows (Qt Creator's default); any newer CMake elsewhere.
   `cmake_minimum_required(VERSION 3.30)`.
3. **Kits and presets:** Qt Creator kits for daily work and **all Android builds**.
   `CMakePresets.json` for command-line, no-UI and Linux builds (`windows-debug`,
   `windows-release`, `windows-noui-debug`, `linux-debug`, `linux-noui-debug`).
   Machine-specific paths (Qt location) live in the git-ignored `CMakeUserPresets.json`.
4. **Build directories:** `build/<preset>/` inside the repo, git-ignored.
5. **Compiler options:** one INTERFACE target `mymoney_compile_options` linked by every own
   target (C++23 without extensions; MSVC `/W4 /permissive- /utf-8 /Zc:__cplusplus
   /Zc:preprocessor`; GCC/Clang `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
   -Wsign-conversion -Wold-style-cast -Wnon-virtual-dtor`). Warnings are errors
   (`COMPILE_WARNING_AS_ERROR`) in own targets only, never in third-party code.
6. **Exceptions** stay enabled in the compiler but are not used in our code (ADR-0008);
   enforced by code review.
7. **GoogleTest v1.18.0** via `FetchContent` from the release archive, pinned by SHA-256,
   included as `SYSTEM`. One test executable per layer, registered with `gtest_discover_tests`.
   Not built for Android.
8. **Line endings:** `.gitattributes` with `* text=auto eol=lf`.
9. **Formatting and analysis:** `.clang-format` based on Google style; `.clang-tidy` with
   Google naming rules; `src/ui/.clang-tidy` inherits it and relaxes rules that conflict with Qt.
   Compile commands are exported for clang-tidy and clangd.
10. **QML:** `mymoney_ui` is a static library with the QML module `MyMoney.UI`; the app links
    its plugin and imports it with `Q_IMPORT_QML_PLUGIN`. The version is passed to `Main.qml`
    as a required property.
11. **Android:** package ID, app name and version are CMake target properties
    (`QT_ANDROID_PACKAGE_NAME`, `QT_ANDROID_APP_NAME`, `QT_ANDROID_VERSION_NAME`,
    `QT_ANDROID_VERSION_CODE`). Version code = `major·10000 + minor·100 + patch`.
    Version single source: `project(MyMoney VERSION …)`.
12. **Names:** targets `mymoney_*`; output files `MyMoney` (app) and `mymoney-cli` (CLI).
    Not `mymoney`: Windows file names are case-insensitive, so `mymoney.exe` and
    `MyMoney.exe` would overwrite each other in one install folder.
    Source files `snake_case.cpp` / `snake_case.h` — **`.cpp`, not Google's `.cc`**.
13. **Workflow:** one branch per milestone (`m0-skeleton`). No GitHub pull requests: the
    changes are reviewed locally, then merged into `master` and pushed.
14. **Desktop deployment:** `cmake --install` (or the `install` target) creates a
    self-contained folder `dist/<preset>/` with both executables plus the Qt libraries,
    plugins and QML modules (Qt's `qt_generate_deploy_qml_app_script`, which runs
    `windeployqt` on Windows). The app runs without Qt on `PATH`.
    The app's QML lives in the `MyMoney.UI` module, so the QML import scanner is pointed at
    `src/ui` — via `QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS` (desktop) and
    `QT_QML_ROOT_PATH` (Android).

## Consequences

- Pure C++ targets are added *before* Qt is set up in the top-level `CMakeLists.txt`, so Qt's
  automatic code generation (AUTOMOC) never touches them.
- Command-line builds with MSVC + Ninja need the MSVC environment: they are run from the
  "Developer PowerShell for VS 2026" (which uses Visual Studio's CMake, ≥ 3.30 as required).
  Qt Creator provides the environment automatically. Presets build into `build/<preset>/`,
  separate from Qt Creator's build folders, so the two CMake versions never share one.
- The Android package ID is fixed from the first install on.
