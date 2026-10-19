# ADR-0015: Category groups (M3b)

- **Status:** Accepted · 2026-10-19
- **Plan items:** D25, M3b
- **Amends:** ADR-0006 (category tree), ADR-0014 (`category add --parent`)

## Context

ADR-0006 allows a category tree of depth 2, and in M3 any category can have both children
and operations. Then "Food" means two things: the food bought without a closer category,
and the total of Food and its children. Reports have to tell them apart, and the user has
to pick between "Food" and "Restaurants" when entering an expense.

Decided 2026-10-18 (option B, "folders and files"): a **group** only contains categories and
never has operations; a **category** has operations and never has children.

## Decision

1. **Rules:**
   - a group is always at the root (so the tree is at most 2 levels deep, as before);
   - a category is at the root or inside a group of the **same kind**;
   - only a category (not a group) can be used in an expense or income;
   - names are unique across groups and categories (the existing `category_name_unique`
     index of migration `0002` already covers this).
2. **Core:** `NewCategory` and `Category` get `bool is_group` (default `false`).
   `CategoryService::Create` checks the structure rules: a group has no parent, a parent
   must be a group of the same kind. `kMaxCategoryDepth` and the depth walk are removed,
   because the rules alone limit the depth. `OperationService` refuses a group as the
   category of an expense or income (`kRuleViolation`).
3. **Storage, migration `0003_category_groups.sql`:**
   - adds `is_group INTEGER NOT NULL DEFAULT 0 CHECK (is_group IN (0, 1))` to `category`;
   - **converts M3 data:** every category that has children becomes a group. If such a
     category also has operations, the migration creates a new category
     `"<name> (other)"` of the same kind in that group and moves the operations there,
     so no balance or total changes;
   - adds triggers (`RAISE(ABORT, ...)`) that refuse: a movement on a group; a group with
     a parent; a parent that is not a group or has a different kind; changing `is_group`
     or `kind` of a category that has children or movements.
   A trigger failure is an unexpected error, so it stays `kStorage` (ADR-0013); the
   services check the same rules first and return `kRuleViolation`.
4. **Fakes mirror the triggers** (contract tests): `InMemoryCategoryRepository::Add`
   checks the structure rules, and `InMemoryOperationRepository` gets a pointer to the
   category fake to refuse movements on groups.
5. **CLI** (confirmed 2026-10-19):
   - `group add <name> --kind expense|income` creates a group (always at the root);
   - `category add <name> [--group NAME] [--kind expense|income]` — `--group` replaces
     `--parent`; with `--group`, `--kind` is optional (taken from the group), and a
     different `--kind` is an error; without `--group`, `--kind` is required;
   - `category list` shows the tree; a group's kind column reads `expense group` /
     `income group`; empty groups are listed too. There is no `group list`;
   - an expense or income on a group fails with exit code 1, and the message lists the
     group's categories.
6. **Not in M3b:** renaming, moving a category into a group, deleting (with editing, M6).

## Consequences

- Every operation points at a leaf category, so reports can sum a group from its
  categories without a special "the group itself" row.
- The existing test database keeps all its operations; one operation on "Food" moves to
  "Food (other)".
- The 1C import (M8) can map 1C catalog groups ("folders") to groups directly, if the old
  data uses them the same way (checked in M8); a 1C level-3 category is still merged into
  its parent (ADR-0006).
