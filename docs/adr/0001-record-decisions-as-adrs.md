# ADR-0001: Record architecture decisions as ADRs

- **Status:** Accepted · 2026-10-13
- **Plan item:** D20

## Context

This is a learning project. Design is discussed first, then the code is written
(see `PROJECT_PLAN.md`, section 0). In a few months it will be hard to remember
*why* a choice was made — only *what* was built will be visible in the code.

## Decision

Every significant decision gets a short Markdown file in `docs/adr/`:
`NNNN-short-title.md` with the sections *Context · Decision · Consequences*.
The decision table in `PROJECT_PLAN.md` (section 11) links to the ADR.

An accepted ADR is not edited later. If a decision changes, a new ADR is written
and the old one gets the status *Superseded by ADR-NNNN*.

## Consequences

- Decisions and their reasons live next to the code, in git history.
- Small overhead: 10–20 lines per decision.
