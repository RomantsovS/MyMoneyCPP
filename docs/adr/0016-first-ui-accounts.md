# ADR-0016: First UI — accounts and balances (M4)

- **Status:** Accepted · 2026-10-21
- **Plan items:** D10, D11, D12, D19, M4
- **Builds on:** ADR-0002 (Qt only in `ui`/`app`), ADR-0012 (domain model), ADR-0013 (storage),
  ADR-0014 (database file location)

## Context

M4 is the first Qt Quick screen that works with real data: the list of accounts with their
balances, and forms to add and edit accounts. It must run on the phone (portrait, touch,
about 400 × 850 dp) and on the desktop in a phone-sized window. Until now the app only shows
its version; all wiring of repositories and services lives in the CLI.

## Decision

1. **Style (D10):** Qt Quick Controls **Material**, light or dark as the system setting says
   (`Material.theme: Material.System`). `Main.qml` becomes an `ApplicationWindow`.
2. **Navigation (D11):** bottom tabs (Accounts · Journal · Reports), each tab with its own
   `StackView`. M4 has only the Accounts tab; the tab bar is shown once a second tab exists.
3. **C++ objects in QML (D12, option B):** the objects QML uses are created in `main.cpp`
   and exposed as `QML_SINGLETON`s whose static `create()` returns that instance (QML does
   not own it). In M4: `AccountListModel`, `CurrencyListModel`, `AccountEditor`,
   `CurrencyEditor`. A form page resets its editor when it opens (`startNew()` /
   `load(accountId)`). `main.cpp` connects the editors' `saved` signals to the models'
   `reload()`. No `setContextProperty`.
4. **Language (D19):** English UI; every visible string in `qsTr()`. A Russian translation
   (Qt Linguist) comes later. Until then the core's error messages are shown as they are;
   mapping error codes to translated texts comes with the translation.
5. **Shared wiring:** `cli::Application` moves to `storage` as **`storage::AppServices`**:
   it owns the `Database`, the SQLite repositories and the core services. The CLI and the
   app each create one in `main`.
6. **Database file:** `QStandardPaths::AppLocalDataLocation` + `mymoney.sqlite`, with no
   organization name set — on Windows and Linux the same file as the CLI (ADR-0014), on
   Android the app's private folder. If the database cannot be opened (e.g. it is newer
   than the app), the app shows the error instead of the UI.
7. **Pages:**
   - *Accounts:* name and balance per account; totals per currency below (no conversion
     before M7); archived accounts hidden unless "Show archived" is on; a `+` button adds an
     account; tapping an account opens it for editing.
   - *Account form:* name; currency (picker, only for a new account — it never changes);
     "+ new" opens the currency form; for an existing account an "Archived" switch.
   - *Currency form:* ISO code and number of decimal places (default 2).
8. **Core additions:** `AccountService::Rename` (name rules as for `Create`) with
   `AccountRepository::Rename`, and `AccountService::Unarchive`; covered by service tests
   and the storage contract tests.
9. **Amounts in the UI:** formatted in C++ from integer minor units with the system
   `QLocale` (its decimal point and group separator, e.g. `1 234,50 EUR` on a Russian
   phone), never through `double`.
10. **Threading:** repository and service calls run on the UI thread. The data is small;
    this is revisited if the journal (M6) becomes slow.
11. **Tests:** a new GoogleTest executable **`mymoney_ui_tests`**, built only with the UI,
    links Qt Test: `QAbstractItemModelTester` checks the list models; the editors and the
    amount formatting are tested against a `:memory:` database. QML is checked with
    `qmllint` (target `all_qmllint`), with 0 warnings like clang-tidy.

## Consequences

- The plan's rule "all unit tests build without Qt" now holds for `core`, `storage` and
  `cli` tests only; `mymoney_ui_tests` needs Qt.
- On Windows and Linux the desktop app and the CLI share one database file.
- Data on the phone is only in the app's private folder until backup exists (M9);
  uninstalling the app deletes it.
- Adding Journal or Reports later means adding a tab with its own stack; the Accounts page
  does not change.
