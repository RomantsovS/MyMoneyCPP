# ADR-0020: Exchange rates and balances in a chosen currency (M7a)

- **Status:** Proposed · 2026-10-27
- **Plan items:** D21, M7a
- **Supersedes:** ADR-0009 (proposed; its rules are kept below where still valid)
- **Builds on:** ADR-0004 (multi-currency), ADR-0012 (domain model), ADR-0013 (storage),
  ADR-0014 (CLI), ADR-0016 (UI)

## Context

M7 is split into M7a–M7f (plan, 2026-10-27): **M7a** brings exchange rates and the current
balances in a chosen currency; M7b–M7e add the old app's features the import (M8) needs —
rates in transfers, a balance check, refunds, debts, joint expenses, labels; M7f is the
income/expense report (per currency, no conversion — ADR-0004). The accounts use **RUB, EUR, USD, KZT and USDT**. No single source has all of them:
the ECB (EUR-quoted) has USD but no RUB (since 2022) and no KZT; the CBR (RUB-quoted) has
RUB, KZT, EUR and USD; nobody official publishes USDT. Today a currency code must be exactly
three letters, so USDT cannot even be created.

## Decision

1. **Sources, chosen per currency automatically:** a currency takes its rate from the **ECB**
   if the ECB feed has it, otherwise from the **CBR** if the CBR feed has it, otherwise it
   needs **manual** rates. EUR is the ECB's own quote currency. With today's currencies:
   USD → ECB; RUB and KZT → CBR; USDT → manual (entered against USD). No setting, no screen.
2. **A rate connects a currency to its quote currency.** One rate per currency and day,
   stored exactly as published but normalized to one unit: *currency*, *date*, *quote
   currency*, *value* (integer, scale 10⁸), the published *direction* (ECB: "1 EUR = 1.0823
   USD"; CBR: "100 KZT = 17.1234 RUB" → 1 KZT = 0.17123400 RUB) and the *source* (`ecb`,
   `cbr`, `manual`). This is lossless for both feeds. RUB is linked to the EUR world by the
   CBR's EUR rate, stored as RUB's rate ("1 EUR = x RUB"). A download never overwrites a
   manual rate.
3. **Conversion:** every currency is followed up its chain of quote currencies — for each
   step the **latest rate on or before** the date (ADR-0009) — to the root (EUR); two
   currencies convert through their common root (e.g. KZT → RUB → EUR → USD). A chain that
   does not reach the root means "no rate". Cycles are refused when a manual rate is entered.
4. **Exact arithmetic:** integers only. Intermediate products need 128 bits (`amount ×
   rate` exceeds `int64`), and MSVC has no `__int128`, so `core` gets a small tested
   multiply-then-divide helper. Each step of a chain keeps 6 extra digits; the result is
   rounded **once**, half away from zero, to the target's minor units.
5. **Balances in a chosen currency:** each currency's total is converted and the results
   are summed. A missing rate is **never 0**: that currency's total stays unconverted and is
   named, and the converted total is marked incomplete ("≈ 12 345.67 EUR, without USDT — no
   rate").
6. **Currency codes:** 3 to 5 letters A–Z (USDT; ISO codes stay valid). Migration `0004`
   rebuilds the `currency` table with the new `CHECK` and adds the `exchange_rate` table.
7. **Downloading (app):** Qt Network (`QNetworkAccessManager`). The feeds are parsed in
   `core` by small hand-written parsers for exactly these two formats (pure C++, tested with
   saved responses; the CBR's windows-1251 only affects currency names, which are skipped).
   Which currencies: those of the active accounts plus the display currency. When: on app
   start if no download succeeded today (asynchronously; a failure shows a status line, not
   a dialog), and with an **Update** button. Only the latest published rates — the history
   comes with the import (M8).
8. **HTTPS on Android:** KDAB's prebuilt OpenSSL for Android (`android_openssl`), pinned
   with CMake `FetchContent`, packaged into the APK. Windows uses Schannel, Linux the
   system's OpenSSL.
9. **CLI (no HTTP):** `rate list`, `rate set` (a manual rate), `rate import <file.xml>`
   (an ECB or CBR file fetched with e.g. `curl`, same parser as the app), and converted
   totals with `--in <currency>` where balances are listed.
10. **UI:** on the Accounts page, under the per-currency totals, one line "≈ total in the
    display currency"; a tap chooses the display currency (remembered with `QSettings`, a UI
    preference). An **Exchange rates** page (`⋮` menu): the latest rate per currency with
    its date and source, Update, and manual entry.
11. **Tests:** the arithmetic helper, rounding, chains and missing rates in `core`; both
    parsers against saved feeds; the rate repository in the contract tests (fake + SQLite),
    including migration 0004 on an existing database; the new CLI commands; the display
    total in `mymoney_ui_tests`. No test touches the network.

## Consequences

- All five currencies get rates; adding a currency later needs no configuration.
- Changing the display currency never changes stored data.
- New dependencies: Qt Network (all platforms) and `android_openssl` (Android, a few MB).
- A source dropping a currency (as the ECB dropped RUB) moves it to the next source on the
  next download, without a code change.
- The income/expense report (M7f) stays per currency; balance history (later) reuses the
  rates and the chain rule.
