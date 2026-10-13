# ADR-0006: Ledger model — operations and movements

- **Status:** Accepted · 2026-10-13
- **Plan item:** D2

## Context

Requirements: income, expense, and transfers between accounts — also between accounts in
different currencies (ADR-0004). Main report: income and expenses by category hierarchy.
Balances per account. Split receipts (one purchase in several categories) are **not** needed.

Options considered: (A) flat table, one row per operation, transfers with
`from_account`/`to_account`; (B) operation header + signed movement lines;
(C) strict double-entry bookkeeping. B is the standard model of personal-finance software
(GnuCash, KMyMoney, Firefly III, hledger) — chosen on its merits, independent of the 1C app.

## Decision

Model **B**:

- **Operation** (what the user enters): id, date, type (`expense` / `income` / `transfer`),
  comment, technical timestamps.
- **Movement** (what the operation changes): operation id, account id, category id (nullable),
  signed amount in minor units, in the currency of the account (`+` in, `−` out).
- Shapes are fixed per operation type and enforced in `core`:

  | Type | Movements |
  |---|---|
  | expense | 1: negative, on an account, with an expense category |
  | income | 1: positive, on an account, with an income category |
  | transfer | 2: negative on the source account, positive on the target account, no category. Same currency → equal amounts. Different currencies → the two amounts are independent. |

- Account balance = sum of its movements.
- Categories form a tree (`parent_id`). Maximum depth **2**, enforced in `core`.
  Level-3 categories from the 1C app are **merged into their parent** during import.

## Consequences

- Cross-currency transfers need no special columns.
- Balances and reports are simple sums over movements.
- Split receipts could be added later by relaxing the shape rules in `core` —
  the schema already allows *n* movements per operation.
- A transfer fee is a separate expense operation.
