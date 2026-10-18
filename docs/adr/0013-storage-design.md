# ADR-0013: Storage design (M2)

- **Status:** Accepted · 2026-10-18
- **Plan items:** D7, M2
- **Builds on:** ADR-0007 (SQLite with an own C++ wrapper), ADR-0012 (domain model)

## Context

M2 implements the repository interfaces of M1 with SQLite. All data of the old app
(since 2012) will live in this database, so its integrity matters more than its speed.

## Decision

1. **SQLite 3.54.1**, the amalgamation fetched with `FetchContent`, pinned by its
   published SHA3-256 hash. Compiled as a separate C library without our warning flags,
   with `SQLITE_DQS=0`, `SQLITE_DEFAULT_FOREIGN_KEYS=1`, `SQLITE_OMIT_LOAD_EXTENSION`,
   `SQLITE_DEFAULT_MEMSTATUS=0`, `SQLITE_OMIT_DEPRECATED`, `SQLITE_OMIT_SHARED_CACHE`.
2. **Wrapper** (`storage/sqlite/`): `Database`, `Statement`, `Transaction` — move-only RAII
   types. `Statement::Bind(args...)` is a variadic template; `Statement::Column<T>(i)` reads
   typed values. Statements are prepared per call (no cache). `sqlite3.h` is not visible
   outside the wrapper's `.cpp` files.
3. **Errors:** every SQLite failure is `kStorage` with SQLite's message. Repositories map the
   cases they expect: no row → `kNotFound`, duplicate currency → `kRuleViolation`.
4. **Schema:** `STRICT` tables; `CHECK` constraints for the most important rules; enums stored
   as TEXT (`'expense'`) through explicit mapping functions; dates as ISO TEXT (`2026-10-18`);
   the currency of a movement is not stored (it is the account's currency).
5. **Migrations (D7):** numbered `.sql` files in `storage/migrations/`, embedded into the
   binary by a CMake step that generates a `.cpp` file. `PRAGMA user_version` holds the
   schema version. Each script and its version update run in one transaction. A database
   newer than the app is refused.
6. **Journal mode `DELETE`:** the database is a single file whenever the app is closed,
   so a backup is a file copy. Foreign keys are switched on for every connection.
7. **Loading goes through the M1 factories** (`OperationDraft::Expense/Income/Transfer`,
   `Currency::Create`, `CurrencyCode::Parse`): a damaged row becomes an error instead of
   silently wrong data.
8. **Contract tests:** the same typed GoogleTest suite runs against the in-memory fakes and
   the SQLite repositories; wrapper and migration tests run on `:memory:` databases.

## Consequences

- The fakes used by the M1 service tests are proven to behave like the real storage.
- The schema defends itself against invalid data from any writer (code, import, manual edits).
- The first configure downloads SQLite (cached afterwards in the build directory).
