# ADR-0019: Journal — browsing, editing and deleting operations (M6)

- **Status:** Accepted · 2026-10-25
- **Plan items:** D28, M6
- **Builds on:** ADR-0006 (operations and movements), ADR-0012 (domain model; editing was
  left for M6), ADR-0013 (storage), ADR-0016 (navigation), ADR-0017 (operation form)

## Context

Operations can be entered on the phone since M5, but not seen, corrected or removed there.
M6 adds the journal. The full history since 2012 arrives in M8, so the design must stay fast
with thousands of operations. No operation can be changed or deleted in the core yet.

## Decision

1. **Navigation:** the Journal is the **second bottom tab** (Accounts · Journal), with its
   own `StackView`; the tab bar is shown from now on (ADR-0016, D11).
2. **One month at a time:** the list shows the operations of one month, starting with the
   current one; `‹ October 2026 ›` moves between months. Newest first, with day headers
   ("Today", "Yesterday", then e.g. "Fri, 23 Oct" in the device's language). A row shows the
   category (or "From → To" for a transfer), the account, the amount with its sign (expenses
   and incomes in their own colours) and the comment.
3. **Filters**, all optional and combinable, in SQL (`OperationFilter`):
   - an account (operations that move money on it — exists since M3);
   - a category, or a group, which means all its categories;
   - **comment text**: SQLite `LIKE '%text%'`. Case is ignored for Latin letters only — a
     SQLite limitation (as with `NOCASE`, migration `0002`). `%` and `_` typed by the user
     match literally (escaped). While a comment text is set, the month is ignored: the whole
     history is searched, newest first, **at most 200 results** (the list says so when it is
     cut).
   The filters are set in a panel opened from the Journal's header; active filters are shown
   as chips that can be removed with one tap.
4. **Core and storage:**
   - `OperationFilter` gains `category_id` (category or group), `comment_contains`, an order
     (oldest first — the CLI's journal — or newest first) and an optional limit;
   - `OperationRepository::Replace(id, draft)` updates the operation row, deletes its
     movements and inserts the new ones in **one transaction**; `Delete(id)` removes an
     operation (its movements follow, `ON DELETE CASCADE`). Both `kNotFound` for an unknown
     id. Covered by the contract tests;
   - `OperationService::Update(id, draft)` checks the new draft like `Post` does;
     `OperationService::Delete(id)`.
5. **Archived accounts freeze their operations:** an operation can be changed or deleted
   only if all of its accounts — before and after the change — are active
   (`kRuleViolation`; unarchive the account to correct one). This extends "archived accounts
   get no new operations" (ADR-0012) to their history.
6. **Editing:** tapping a row opens the operation form of M5 in edit mode, filled with the
   operation's values; its type cannot change. Save updates it; a **Delete** button asks for
   confirmation first. Afterwards the journal and the balances are reloaded, and a short
   message says what happened ("Expense 12.50 EUR deleted").
7. **C++ in `mymoney_ui`:** a `JournalModel` (month, filters, a day role for the `ListView`
   sections, reset after every change); `OperationEditor` gains `load(id)` and
   `deleteOperation()`.
8. **Not in M6:** swipe to delete, Unicode-aware case folding in the search, CLI commands
   for editing or deleting.
9. **Tests:** the new filters, `Replace` (including that a refused change leaves the old
   operation intact) and `Delete` in the contract tests; `Update`/`Delete` rules (archived
   accounts) in the service tests; `JournalModel` and the editor's edit mode in
   `mymoney_ui_tests`.

## Consequences

- The phone shows, corrects and removes past operations; balances follow every change.
- A month query stays small; a comment search over the whole history is bounded by its
  limit.
- Searching Cyrillic comments ignores case only after a later change (e.g. a folded search
  column or an SQLite function written in C++).
