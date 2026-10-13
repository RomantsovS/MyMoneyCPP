# ADR-0005: App name and Android package ID

- **Status:** Accepted · 2026-10-13
- **Plan item:** D18

## Decision

- App name: **MyMoney**.
- Android package ID: **`io.github.romantsovs.mymoney`** — reverse domain of the GitHub Pages
  address `romantsovs.github.io`, which belongs to the GitHub account.
- Repository: GitHub `RomantsovS/MyMoneyCPP`, default branch `master`.

## Consequences

The package ID must never change after the first install: Android treats a different ID
as a different app with separate data.
