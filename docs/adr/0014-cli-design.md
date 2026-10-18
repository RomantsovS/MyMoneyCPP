# ADR-0014: Command-line interface design (M3)

- **Status:** Accepted · 2026-10-18
- **Plan items:** D22, M3
- **Builds on:** ADR-0010 (CLI frontend), ADR-0012 (domain model), ADR-0013 (storage)

## Context

M3 is the first end-to-end frontend: commands typed by the user go through `core` into a
real SQLite file. It must be usable for a month of real data and fully testable.

## Decision

1. **Scope:** currencies (add, list), accounts (add, list, archive), categories (add, list as
   a tree), expense / income / transfer, journal, balance. The income/expense report comes
   with exchange rates in M7.
2. **Argument parsing (D22):** a hand-written parser — positionals plus `--name value` /
   `--name=value` options — and a command table (name, arguments, options, usage, handler).
   `--help` is generated from the table. No third-party parser: the common ones (CLI11,
   cxxopts, argparse) report errors with exceptions (ADR-0008).
3. **Names instead of ids:** commands refer to accounts and categories by name.
   **Account names and category names are unique — each globally, case-sensitive** (also
   across expense and income categories). The services check it (`kRuleViolation`), and
   `UNIQUE` indexes in migration `0002` enforce it in the database.
4. **Amounts as text** in `core/amount_text.h`: `ParseAmount` accepts `.` or `,` as decimal
   separator, at most the currency's number of decimals, no sign, no thousands separators,
   up to the input limit; `FormatAmount` writes a fixed, locale-independent format
   (`-1234.50`). Locale-aware formatting belongs to the Qt UI.
5. **Dates:** ISO `YYYY-MM-DD` text, parsed and formatted in `core/date_text.h` (storage
   reuses it). `--date` defaults to today's local date.
6. **Journal:** new query `OperationRepository::List(OperationFilter)` — optional date range
   and account; ordered by date, then id. Name lookups `AccountRepository::GetByName` and
   `CategoryRepository::GetByName`. All covered by the storage contract tests.
7. **Database file:** `--db PATH`, otherwise the per-user app data folder —
   `%LOCALAPPDATA%\MyMoney\mymoney.sqlite` on Windows,
   `$XDG_DATA_HOME/MyMoney/mymoney.sqlite` (or `~/.local/share/...`) on Linux —
   the same place the Qt app will use. The folder is created if needed.
8. **Structure:** commands return their output as text (`Result<std::string>`); only `main`
   prints. Exit codes: `0` success, `1` an error from domain or storage, `2` wrong usage.
   The CLI code is a library (`mymoney_cli_lib`) plus a thin `main`, so tests can run real
   commands against a `:memory:` database and compare the output.
9. **Windows text encoding:** the CLI executable declares UTF-8 as its active code page in
   an application manifest, so command-line arguments arrive as UTF-8 (names in any
   language) and match what is stored.

## Consequences

- An account or a category is identified by its name everywhere in the CLI.
- Duplicate category names in the old app's data must be renamed during import (M8).
- Almost all CLI code is covered by fast tests; `main` only wires and prints.
