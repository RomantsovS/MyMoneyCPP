# ADR-0004: Multiple currencies from the start

- **Status:** Accepted · 2026-10-13
- **Plan item:** D3

## Context

The existing data (since 2012) has accounts in different currencies.
Adding currency support to a single-currency model later would change every amount
in the schema and the code.

## Decision

- Every account has exactly one currency (ISO 4217 code, e.g. `EUR`, `USD`).
- Every amount is a `Money` value: integer minor units + currency. Arithmetic is allowed only
  between amounts of the same currency.
- A transfer between accounts in different currencies stores **two amounts**
  (what left one account, what arrived in the other).
- Reports show totals **per currency**. Conversion to one main currency with exchange rates
  is *not* in the MVP.

## Consequences

- No exchange-rate tables, no conversion logic in the MVP.
- A "total of everything in one currency" view needs rates — a later feature.
