# ADR-0017: Entering operations in the UI (M5)

- **Status:** Accepted · 2026-10-25
- **Plan items:** D26, M5
- **Builds on:** ADR-0006 (operation shapes), ADR-0014 (amount text), ADR-0015 (category
  groups), ADR-0016 (first UI)

## Context

M5 makes the phone app usable for daily entry: expenses, income and transfers. The goal
from the plan is that **adding an expense takes less than 10 seconds**, so the common case
must need only an amount and one tap for the category. The core rules (operation shapes,
archived accounts, category kinds, groups) already exist and are not repeated in the UI.

## Decision

1. **Entry point:** the `+` button on the Accounts page opens **New operation**;
   "New account" moves into the page's `⋮` menu.
2. **One form, one controller:** a single page with an Expense / Income / Transfer switch at
   the top, backed by one `OperationEditor` (QML singleton created in `main`, like the
   M4 editors). Fields:
   - expense / income: amount, category, account, date, comment;
   - transfer: amount, from account, to account, date, comment, and a second amount
     ("received") only when the two accounts have different currencies — the same rule as
     the CLI's `--amount-in` (ADR-0006).
   The amount field has the focus when the page opens, so the keyboard is already shown.
3. **Pre-filled values come from the data:**
   - the account is the one of the most recently entered operation of that type (for a
     transfer: its from and to accounts), unless that account is archived;
   - up to 6 **recent categories** are offered: those of the most recently entered
     operations of that type, newest first.
   Two new repository queries provide this, covered by the storage contract tests:
   `OperationRepository::Latest(OperationType)` (`kNotFound` if there is none) and
   `OperationRepository::RecentCategories(OperationType, limit)`. "Most recent" means the
   last one entered (highest id), not the latest date.
4. **Category picker:** the recent categories are one-tap chips on the form; `▸` opens a
   picker page with all categories of the operation's kind, grouped by group (groups are
   section headers that cannot be chosen, ADR-0015) and a filter field.
   **Account picker:** active accounts only.
5. **Amount:** a text field with the system number keyboard (`inputMethodHints`). The
   controller parses it with `ParseAmount` and the chosen account's number of decimal places
   (`.` or `,`); the Save button is enabled only for a valid amount.
6. **Date:** "Today" (the default) and "Yesterday" chips, and a calendar dialog
   (`MonthGrid`) for any other date. QML passes a date to C++ as year, month and day numbers,
   never as a JavaScript `Date` (which is a time in a time zone). The controller converts to
   `std::chrono::year_month_day`.
7. **After Save:** back to the Accounts page, whose balances are reloaded (`main` connects
   the editor's `saved` signal), with a short message at the bottom, e.g.
   "Expense 12.50 EUR saved".
8. **Errors:** the core checks everything again on save (`OperationService::Post`); its
   message is shown in the form, as in M4.
9. **Tests:** the two queries in the contract tests (fakes and SQLite); `OperationEditor`
   and the picker model in `mymoney_ui_tests` (expense, income, transfer in one and in two
   currencies, defaults, invalid input).

## Consequences

- With an amount and a recent category, an expense takes about five taps and keystrokes.
- Operations entered with the CLI also feed the pre-filled values.
- There is still no list of operations in the UI; the journal follows in M6.
