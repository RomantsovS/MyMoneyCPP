# ADR-0007: SQLite with an own C++ wrapper behind repository interfaces

- **Status:** Accepted · 2026-10-13
- **Plan item:** D6

## Context

All data must stay local on the phone, in one file that can be backed up.
The data is relational (operations → accounts, categories; categories form a tree), and the
main reports aggregate across all operations. A transfer changes two accounts atomically.
Size: about 14 years of history, tens of thousands of operations — a few MB.
"As little Qt as possible" is a project goal (ADR-0002).

MongoDB was considered and rejected: it is a server with no Android version
(its mobile database, Realm / Atlas Device SDK, was deprecated in 2024), and a document
model does not fit cross-cutting aggregations over relational data.

## Decision

- Storage engine: **SQLite**, embedded as the amalgamation (`sqlite3.c`), pinned version,
  compiled from source for every platform.
- Access: an **own C++23 wrapper** over the SQLite C API in the `storage` layer —
  RAII types for the connection, prepared statements and transactions;
  errors as `std::expected`. **No Qt SQL.**
- `core` defines **repository interfaces** in pure C++. `storage` implements them.
  `core` does not know that SQL exists.

## Consequences

- `core` and `storage` are pure C++23 + SQLite; Qt is needed only for `ui` and `app`.
- `storage` is tested against SQLite `:memory:` databases; `core` with in-memory fakes.
- The storage technology can be replaced later by writing a new implementation of the
  repository interfaces.
- More code to write than with Qt SQL — which is part of the learning goal.
