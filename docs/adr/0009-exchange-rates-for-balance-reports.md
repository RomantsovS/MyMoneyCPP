# ADR-0009: Exchange rates for balance reports

- **Status:** Proposed · 2026-10-13, updated 2026-10-16 — one open question (rate source)
- **Plan item:** D21
- **Amends:** ADR-0004 (which said "no conversion in the MVP")

## Context

The income/expense report is shown **per currency**, without conversion (ADR-0004).
The *current balances* report must be shown in **any chosen currency, with conversion**.
*Balance history* is **not** in the MVP.

The old app downloaded rates from the internet into a register: the list of currencies to
download was configurable, downloading was possible manually or automatically on app start.
Its rates were quoted against the Russian rouble. The rate history since 2012 is migrated (M8).

## Decision (proposed)

**Rates are stored as published by their source — there is no global "base currency" setting.**

- A rate row: date, currency, **quote currency** (the currency the source quotes against),
  rate value, and units (nominal, e.g. "100 JPY = …"). Stored **exactly**: integer-scaled
  decimal, never `double`.
- **Display currency** is a user choice per report, independent of how rates are stored.
  Conversion between two currencies quoted against the same quote currency uses the
  cross rate. Converted amounts are rounded to minor units only for display.
- For a given date, the rate used is the **latest rate on or before** that date.
- **Downloading:** from one public source; the list of currencies is configurable;
  manually, and automatically on app start (asynchronous, only if today's rates are missing;
  no Android background service).
- **Structure:** `core` defines a `RateSource` interface and parses the source's response
  format (pure C++, tested with saved responses). The HTTP transport is provided by each
  frontend (Qt Network in the app; CLI transport decided in M7).
- Manual rate entry as a fallback (when offline or for a currency the source lacks).
- The income/expense report stays per currency.

## Open questions (decide before M7)

1. **Rate source** — depends on the currencies held. The Central Bank of Russia (quote: RUB,
   free XML, long history) is consistent with the imported history. The ECB (quote: EUR)
   has no RUB rates since March 2022.
2. Response parsing (XML) and CLI HTTP transport — technology choice in M7.

## Consequences

- New `exchange_rate` table, a conversion service and a `RateSource` interface in `core`.
- Changing the display currency never changes stored data.
- Balance history (later) reuses the same rates and lookup rule.
