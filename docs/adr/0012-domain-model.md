# ADR-0012: Domain model and core design (M1)

- **Status:** Accepted · 2026-10-18
- **Plan items:** D4, D23, M1

## Context

M1 implements the ledger rules of ADR-0006 in `mymoney_core` — pure C++23 (ADR-0002),
no exceptions (ADR-0008), no storage yet (SQLite comes in M2, ADR-0007).

## Decision

1. **Scope:** value types, entities, operation shape rules, category rules, repository
   interfaces, `AccountService`, `CategoryService`, `OperationService`, `BalanceService`.
   Not in M1: SQLite, parsing/formatting amounts like "12.34" (M3), exchange rates (M7),
   editing/deleting operations (M6).
2. **`CurrencyCode`:** three uppercase ASCII letters in a `std::array<char, 3>`.
   `CurrencyCode::Parse()` for runtime text; a `consteval` constructor for literals, so an
   invalid literal does not compile. The number of minor digits belongs to a `Currency`
   entity (`{code, minor_digits}`), not to `Money`.
3. **`Money`:** `int64` minor units + `CurrencyCode`, may be negative. Mixing currencies in
   arithmetic or comparison is a **precondition violation** (a programming error), as is
   `int64` overflow. Amounts entered by the user are limited to `|amount| ≤ 10^15` minor units.
4. **Strong IDs:** `StrongId<Tag>` wraps an `int64`; `AccountId`, `CategoryId`,
   `OperationId` cannot be mixed up. They have no default value.
5. **IDs (D4):** `int64`, assigned by the storage (SQLite `INTEGER PRIMARY KEY`).
6. **Unsaved vs saved objects:** separate types — `NewAccount` / `Account`,
   `NewCategory` / `Category`, `OperationDraft` / `Operation`. A saved object always has an ID;
   an unsaved one has none.
7. **Two-level validation:**
   - *intrinsic* rules (need only the object): checked by factory functions —
     `OperationDraft::Expense/Income/Transfer` enforce the shapes of ADR-0006;
   - *contextual* rules (need other data): checked by services through repositories —
     account exists and is not archived, currencies match, category kind, category depth.
8. **Repositories:** abstract classes with virtual functions. Balances are computed by a
   query (`SumMovements`) — a SQL `SUM` in M2 — not by loading all movements. Services hold
   repositories as non-owning, non-null raw pointers (reference members are forbidden by
   `cppcoreguidelines-avoid-const-or-ref-data-members`); the composition root owns them.
   `OperationRepository::Add` stores an operation and its movements atomically.
9. **Errors:** `Error { ErrorCode code; std::string message; }` with codes
   `kInvalidArgument` (malformed input), `kNotFound`, `kRuleViolation` (valid input that breaks
   a business rule), `kStorage`; `Result<T> = std::expected<T, Error>`,
   `Status = std::expected<void, Error>`. Messages are for developers; the UI maps codes to
   user-facing texts.
10. **Precondition checks:** a small `Check()` function that reports the failed condition with
    its source location and aborts — **also in Release builds** (an `assert` would disappear
    there and let a corrupted balance continue silently). Same idea as Google's `CHECK`.
11. **Logging (D23):** none in `core` and `storage`; frontends log the errors they receive.
12. **Tests:** hand-written in-memory fake repositories (state-based tests); GoogleMock only
    where an interaction itself matters.

## Consequences

- Swapped IDs, unsaved objects used as saved ones, and invalid currency literals are compile
  errors.
- Every `OperationDraft` that exists is well-formed; services only check what needs other data.
- `core` stays testable without a database; M2 implements the same interfaces with SQLite.
