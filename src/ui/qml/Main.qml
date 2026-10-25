import QtQuick
import QtQuick.Controls.Material

import MyMoney.UI

// The main window (ADR-0016): Material style, light or dark as the system
// says; bottom tabs, each tab with its own page stack: Accounts and, since
// M6, Journal (ADR-0019).
ApplicationWindow {
    id: root

    // Passed in from C++ (src/app/main.cpp) with setInitialProperties().
    required property string appVersion

    width: 400
    height: 850
    visible: true
    title: qsTr("MyMoney %1").arg(root.appVersion)

    Material.theme: Material.System
    // The colour of the active tab and its underline, day headers, the +
    // button and chosen chips. Without it Material's default accent looks grey
    // in the dark theme. (In the dark theme Material uses a lighter shade.)
    Material.accent: Material.Teal

    // The page stack of the tab on screen.
    readonly property StackView currentStack: tabs.currentIndex === 0 ? accountsStack : journalStack

    // A closing dialog leaves the focus on the window's root item (the only
    // item without a parent) for a moment, outside the pages that handle the
    // back key - the release of the back that closed it would land there (see
    // acceptBackRelease()). The focus goes straight back to the pages.
    onActiveFocusItemChanged: {
        if (activeFocusItem !== null && activeFocusItem.parent === null)
            pages.forceActiveFocus();
    }

    footer: TabBar {
        id: tabs

        visible: count > 1
        // Kept in sync with the swipe view both ways.
        currentIndex: pages.currentIndex
        // Android draws the app edge to edge, under its navigation bar. The
        // window keeps only its content out of the system bars, not the
        // footer, and Material's TabBar - unlike its ToolBar - ignores them.
        // So the tabs keep clear of them here; the background still reaches
        // the screen edge. (Zero on the desktop.)
        leftPadding: SafeArea.margins.left
        rightPadding: SafeArea.margins.right
        bottomPadding: SafeArea.margins.bottom
        // The tab bar is outside the swipe view: a back key that starts here
        // must be handled here too (see goBack()).
        Keys.onBackPressed: root.goBack()
        Keys.onReleased: event => root.acceptBackRelease(event)

        // A tap on a tab must not take the keyboard focus away from the
        // pages, where the back key is handled.
        TabButton {
            text: qsTr("Accounts")
            focusPolicy: Qt.NoFocus
        }
        TabButton {
            text: qsTr("Journal")
            focusPolicy: Qt.NoFocus
        }
    }

    // Tabs switch with a tap on the tab bar or a swipe anywhere on a tab's
    // first page. On a sub-page (e.g. the operation form) swiping is off, so
    // a sideways gesture there cannot throw away what is being edited.
    SwipeView {
        id: pages

        anchors.fill: parent
        currentIndex: tabs.currentIndex
        interactive: root.currentStack.depth === 1

        // Android's back gesture (and back button) arrives as Qt.Key_Back at
        // the item with the keyboard focus and bubbles up to here. In Qt 6.12
        // a back key that no item accepts moves the app to the background -
        // the window gets no close request - so it is handled here. An open
        // dialog closes itself on back first (Popup.CloseOnEscape).
        focus: true
        Keys.onBackPressed: root.goBack()
        Keys.onReleased: event => root.acceptBackRelease(event)

        // After Android's back gesture - whose touch Android takes back from
        // the app - the swipe view's list stops following its index with an
        // animation (it never sees that touch end). So every index change
        // that the user is not swiping right now places the list directly.
        onCurrentIndexChanged: {
            const list = contentItem as ListView;
            if (!list.moving)
                list.positionViewAtIndex(currentIndex, ListView.Beginning);
        }

        StackView {
            id: accountsStack

            initialItem: AccountsPage {}
        }

        StackView {
            id: journalStack

            initialItem: JournalPage {}
        }
    }

    // A short message after an operation was saved (ADR-0017), shown over
    // whatever page the user returns to.
    ToolTip {
        id: savedMessage

        x: (root.width - width) / 2
        y: root.height - height - 96
        timeout: 3000
    }

    Connections {
        target: OperationEditor

        function onSaved() {
            savedMessage.text = OperationEditor.savedText;
            savedMessage.open();
        }
    }

    // Back: one page back in the current tab; on the Journal's first page to
    // Accounts; on Accounts a hint first, and a second back within two
    // seconds closes the app.
    function goBack() {
        if (root.currentStack.depth > 1) {
            root.currentStack.pop();
        } else if (pages.currentIndex !== 0) {
            pages.setCurrentIndex(0);  // keeps the bindings to the tab bar
        } else if (exitTimer.running) {
            // Ends the app: Qt then finishes the Android activity and exits
            // the process; main() closes the database on the way out.
            Qt.quit();
        } else {
            exitTimer.start();
            savedMessage.text = qsTr("Go back again to exit");
            savedMessage.open();
        }
    }

    // Qt moves the app to the background when neither the press nor the
    // release of the back key was accepted. Qt 6.12 sometimes reports a
    // handled press as not accepted (a race between Android's thread and
    // Qt's), so the release is accepted too: then both reports would have to
    // be wrong at once.
    function acceptBackRelease(event: KeyEvent) {
        if (event.key === Qt.Key_Back)
            event.accepted = true;
    }

    Timer {
        id: exitTimer

        interval: 2000
    }
}
