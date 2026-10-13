# ADR-0009: Exchange rates for balance reports

- **Status:** Proposed · 2026-10-13 — open questions below
- **Plan item:** D21
- **Amends:** ADR-0004 (which said "no conversion in the MVP")

## Context

The income/expense report is shown **per currency**, without conversion (ADR-0004).
But balance reports — *current balances* and *balance history* — must be shown in
**any chosen currency, with conversion**. Conversion needs exchange rates, per date.

## Decision (proposed)

- Exchange rates are stored per **date** and **currency**, relative to one **base currency**.
  Conversion between two non-base currencies uses the cross rate through the base currency.
- For a given date, the rate used is the **latest rate on or before** that date.
- Rates are stored **exactly** (integer-scaled decimal or numerator/denominator — not `double`).
  Converted amounts are rounded to minor units only for display.
- Balance at a date = sum of movements up to that date, converted with the rate of that date.
- The income/expense report stays per currency.

## Open questions

1. **Source of rates:** import the history from the 1C app (does it have a rates register?),
   manual entry, automatic download from a public source (needs network access) — or a mix?
2. **Base currency:** which one?
3. **MVP scope:** current balances with conversion — MVP? Balance history — MVP or later?
4. **Precision:** how many decimal places do the existing rates have?

## Consequences

- New `exchange_rate` table and a conversion service in `core`.
- A balance can change over time without any operation, only because the rate changed —
  balance history shows this explicitly.
