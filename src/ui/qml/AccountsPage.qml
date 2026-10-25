// The delegate below uses ids of this file (page, accountForm): "Bound" ties
// every inline component to the scope it is declared in.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The Accounts tab (ADR-0016): every account with its balance and the totals
// per currency below. Tap an account to edit it, + to enter an operation;
// new accounts are added from the menu.
Page {
    id: page

    title: qsTr("Accounts")

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            Label {
                text: page.title
                font.pixelSize: 20
                elide: Label.ElideRight
                Layout.leftMargin: 16
                Layout.fillWidth: true
            }

            ToolButton {
                id: menuButton

                text: "⋮"
                onClicked: menu.open()

                Menu {
                    id: menu

                    // A back gesture's first touch must not close the menu
                    // (see JournalFilter.qml); back itself closes it.
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnReleaseOutside
                    y: menuButton.height

                    MenuItem {
                        text: qsTr("New account")
                        onTriggered: page.StackView.view.push(accountForm, {
                            accountId: -1
                        })
                    }
                    MenuItem {
                        text: qsTr("Categories")
                        onTriggered: page.StackView.view.push(categoriesPage)
                    }
                    MenuItem {
                        text: qsTr("Show archived")
                        checkable: true
                        checked: AccountListModel.showArchived
                        onToggled: AccountListModel.showArchived = checked
                    }
                }
            }
        }
    }

    ListView {
        id: list

        anchors.fill: parent
        clip: true
        model: AccountListModel

        delegate: ItemDelegate {
            id: row

            // Filled from the model's roles (AccountListModel::roleNames).
            required property double accountId
            required property string name
            required property string balanceText
            required property bool archived

            width: ListView.view.width
            opacity: row.archived ? 0.5 : 1

            contentItem: RowLayout {
                Label {
                    text: row.name
                    elide: Label.ElideRight
                    Layout.fillWidth: true
                }
                Label {
                    text: row.balanceText
                }
            }

            onClicked: page.StackView.view.push(accountForm, {
                accountId: row.accountId
            })
        }
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - 32
        visible: list.count === 0 && AccountListModel.errorText === ""
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: qsTr("No accounts yet. Tap + to add one.")
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - 32
        visible: AccountListModel.errorText !== ""
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Material.color(Material.Red)
        text: AccountListModel.errorText
    }

    RoundButton {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        width: 56
        height: 56
        highlighted: true
        font.pixelSize: 24
        text: "+"
        // The most frequent action: a new operation (ADR-0017).
        onClicked: page.StackView.view.push(operationForm)
    }

    // One line per currency; hidden while there are no accounts.
    footer: Pane {
        visible: AccountListModel.totals.length > 0

        ColumnLayout {
            anchors.fill: parent

            Repeater {
                model: AccountListModel.totals

                delegate: Label {
                    required property string modelData

                    Layout.alignment: Qt.AlignRight
                    font.bold: true
                    text: qsTr("Total %1").arg(modelData)
                }
            }
        }
    }

    Component {
        id: accountForm

        AccountForm {}
    }

    Component {
        id: operationForm

        OperationForm {}
    }

    Component {
        id: categoriesPage

        CategoriesPage {}
    }
}
