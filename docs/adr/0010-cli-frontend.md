# ADR-0010: A command-line frontend for PC

- **Status:** Accepted · 2026-10-13
- **Plan item:** D22

## Context

The graphical UI (Qt Quick) is the main frontend, made for Android.
On PC a command-line interface is wanted too — for daily use, scripting and testing.

## Decision

- A second executable, **`mymoney_cli`**, built on `core` + `storage`, in **pure C++** (no Qt).
- Desktop only (Windows, Linux) — not built for Android.
- It comes **before** the graphical UI in the roadmap: every feature works first through the CLI.
- The import from the 1C app becomes a CLI command (instead of a separate tool).
- Command-line parsing library: open question (D22 in the plan).

## Consequences

- Two composition roots: `src/app/main.cc` (Qt) and `src/cli/main.cc` (pure C++).
- The CLI keeps the design honest: business logic cannot leak into QML,
  because everything must also work without it.
- The data layer can be used and tested with real data before any UI exists.
