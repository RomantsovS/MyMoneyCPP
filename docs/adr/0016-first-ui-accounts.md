# ADR-0016: First UI — accounts and balances (M4)

- **Status:** Accepted · 2026-10-21 · amended 2026-10-25 (see the end)
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

## Amendment · 2026-10-25 (M6)

Found on the phone once the Journal tab existed:

- **Accent colour:** `Material.accent: Material.Teal` on the main window. The active tab uses
  the accent colour; the default accent looked grey in the dark theme, so the active tab
  seemed less visible than the inactive ones.
- **Swipe between tabs:** a `SwipeView` replaces the `StackLayout` (decision 2). Tabs switch
  by tapping the tab bar or by swiping anywhere on a tab's first page. On a sub-page — for
  example the operation form — swiping is off, so a sideways gesture cannot leave a form
  that is being edited.
- **Back gesture / back button:** handled in QML with `Keys.onBackPressed` on the swipe view,
  not with `ApplicationWindow.onClosing`. In Qt 6.12 on Android a back key that no item
  accepts moves the app to the background without a close request (`androidjniinput.cpp`),
  so `onClosing` never ran on the phone. Back goes one page back in the current tab; on the
  Journal's first page it switches to Accounts; on Accounts a first back shows "Go back again
  to exit", and a second one within two seconds closes the app (`Qt.quit()`: Qt finishes the
  activity and ends the process). Dialogs set `focus: true`, so they get the key and close
  themselves first (a popup handles back only with the focus). Tab buttons do not take the
  focus. After the back *gesture* — whose touch Android takes back from the app — the swipe
  view no longer animates to a new index (a tab tap or the switch to Accounts), so every
  index change that is not a swipe in progress places it with `positionViewAtIndex()`; a
  tab change therefore jumps instead of sliding.
- **Back key release:** Qt 6.12 sometimes reports a handled back *press* as not accepted (a
  race between Android's thread and Qt's GUI thread in `flushWindowSystemEvents()`), and then
  moves the app to the background after all — about one back in 30 in the emulator tests.
  The pages therefore accept the back key's *release* too (Qt backgrounds the app only when
  both look unaccepted), and when a closing dialog leaves the focus on the window's root
  item, the focus goes straight back to the pages, so that release is accepted as well.
  Reported to Qt with the analysis.
- **Navigation bar:** Android draws the app edge to edge. `ApplicationWindow` keeps only its
  content out of the system bars, and Material's `TabBar` (unlike its `ToolBar`) ignores
  them, so with 3-button navigation the tab bar lay under the system buttons. The tab bar's
  padding follows `SafeArea.margins`; its background still reaches the screen edge.
