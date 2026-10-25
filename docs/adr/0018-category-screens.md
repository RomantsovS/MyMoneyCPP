# ADR-0018: Category screens in the app (M5b)

- **Status:** Accepted · 2026-10-25
- **Plan items:** D27, M5b
- **Builds on:** ADR-0015 (category groups), ADR-0016 (first UI), ADR-0017 (operation form)

## Context

Since M3 categories and groups can only be created with the CLI. The phone has its own
database (app-private storage), so on the phone there are no categories at all, and only
transfers can be entered. Categories are part of the MVP (plan section 3.1), but no
milestone planned screens for them. M5b closes that gap before the journal (M6).

## Decision

1. **Scope:** list groups and categories, add a group, add a category (at the root or in a
   group of the same kind), rename both. No delete and no moving between groups yet: both
   need rules about existing operations first.
2. **Core:** `CategoryService::Rename` (the name rules of `Create`: not blank, unique across
   groups and categories, ADR-0014) and `CategoryRepository::Rename`, covered by service
   tests and the storage contract tests. No migration: the existing unique index refuses a
   duplicate name, and no trigger of migration `0003` concerns the name.
3. **Categories page:** reached from the Accounts page menu (`⋮` → Categories). An
   Expense / Income switch at the top; below it the tree of that kind: categories at the root
   first, then every group (also an empty one) followed by its categories, indented. Tapping
   a row opens it for renaming. Buttons "+ Group" and "+ Category".
4. **Category form:** a new category has a name and a group picker ("No group" or a group of
   the chosen kind); a new group has only a name; an existing one can only be renamed (its
   kind and group stay).
5. **From the operation form:** the category picker gets "+ New category" at the top. It
   opens the category form with the operation's kind; after saving, the new category is
   chosen for the operation and the user is back on the operation form.
6. **C++ in `mymoney_ui`:** a `CategoryTreeModel` for the page (rows with id, name,
   is-group, indented) and a `CategoryEditor` (like `AccountEditor`: properties, `valid`,
   `errorText`, `save()`, a `saved` signal with the new id). `main` reloads the tree and the
   picker after a save.
7. **Tests:** rename in the service and contract tests; the tree model (with
   `QAbstractItemModelTester`) and the editor in `mymoney_ui_tests`.

## Consequences

- A fresh phone can be set up completely in the app: currency, accounts, categories.
- A typo in a category name no longer needs the CLI.
- Deleting categories and moving them between groups remain open (later, with rules for
  their operations).
