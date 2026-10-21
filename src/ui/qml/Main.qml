import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The main window (ADR-0016): Material style, light or dark as the system
// says; bottom tabs, each tab with its own page stack. M4 has only the
// Accounts tab, so the tab bar stays hidden until a second tab exists.
ApplicationWindow {
    id: root

    // Passed in from C++ (src/app/main.cpp) with setInitialProperties().
    required property string appVersion

    width: 400
    height: 850
    visible: true
    title: qsTr("MyMoney %1").arg(root.appVersion)

    Material.theme: Material.System

    footer: TabBar {
        id: tabs

        visible: count > 1

        TabButton {
            text: qsTr("Accounts")
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        StackView {
            id: accountsStack

            initialItem: AccountsPage {}
        }
    }

    // Android's back button closes the window; inside a tab it should go back
    // one page first.
    onClosing: close => {
        if (accountsStack.depth > 1) {
            close.accepted = false;
            accountsStack.pop();
        }
    }
}
