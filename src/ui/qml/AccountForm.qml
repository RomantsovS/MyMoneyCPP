import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The account form (ADR-0016): a new account (name, currency) or an existing
// one (name, archived - the currency never changes). The state lives in the
// AccountEditor singleton; this page only binds to it.
Page {
    id: page

    // -1: a new account; otherwise the id of the account to edit. Set by the
    // page that pushes this one.
    property double accountId: -1

    title: AccountEditor.isNew ? qsTr("New account") : qsTr("Account")

    Component.onCompleted: {
        if (page.accountId < 0)
            AccountEditor.startNew();
        else
            AccountEditor.load(page.accountId);
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "‹"
                font.pixelSize: 24
                onClicked: page.StackView.view.pop()
            }
            Label {
                text: page.title
                font.pixelSize: 20
                elide: Label.ElideRight
                Layout.fillWidth: true
            }
            ToolButton {
                text: qsTr("Save")
                enabled: AccountEditor.valid
                onClicked: {
                    if (AccountEditor.save())
                        page.StackView.view.pop();
                }
            }
        }
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 12

        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr("Name")
            text: AccountEditor.name
            onTextEdited: AccountEditor.name = text
        }

        RowLayout {
            visible: AccountEditor.isNew
            Layout.fillWidth: true

            ComboBox {
                id: currencyBox

                Layout.fillWidth: true
                model: CurrencyListModel
                textRole: "code"
                valueRole: "code"
                currentIndex: indexOfValue(AccountEditor.currencyCode)
                displayText: currentIndex < 0 ? qsTr("Currency") : currentText
                onActivated: AccountEditor.currencyCode = currentValue
            }
            Button {
                text: qsTr("+ New")
                onClicked: page.StackView.view.push(currencyForm)
            }
        }

        Label {
            visible: AccountEditor.isNew && currencyBox.count === 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Add a currency first: tap \"+ New\".")
        }

        Label {
            visible: !AccountEditor.isNew
            text: qsTr("Currency: %1").arg(AccountEditor.currencyCode)
        }

        Switch {
            visible: !AccountEditor.isNew
            text: qsTr("Archived")
            checked: AccountEditor.archived
            onToggled: AccountEditor.archived = checked
        }

        Label {
            visible: AccountEditor.errorText !== ""
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Material.color(Material.Red)
            text: AccountEditor.errorText
        }
    }

    // A currency added from here is selected right away. main.cpp reloads
    // CurrencyListModel first (its connection was made earlier).
    Connections {
        target: CurrencyEditor

        function onSaved(code: string) {
            AccountEditor.currencyCode = code;
            currencyBox.currentIndex = currencyBox.indexOfValue(code);
        }
    }

    Component {
        id: currencyForm

        CurrencyForm {}
    }
}
