// The delegate below uses ids of this file: "Bound" ties every inline
// component to the scope it is declared in.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The account picker (ADR-0017): the active accounts.
Page {
    id: page

    // "from": the account of an expense or income, or a transfer's source;
    // "to": a transfer's target. Set by the page that pushes this one.
    property string target: "from"

    title: page.target === "to" ? qsTr("To account") : qsTr("Account")

    Component.onCompleted: AccountPickerModel.reload()

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
        }
    }

    ListView {
        id: list

        anchors.fill: parent
        clip: true
        model: AccountPickerModel

        delegate: ItemDelegate {
            id: row

            // Filled from the model's roles (AccountPickerModel).
            required property double accountId
            required property string name
            required property string currency

            width: ListView.view.width

            contentItem: RowLayout {
                Label {
                    text: row.name
                    elide: Label.ElideRight
                    Layout.fillWidth: true
                }
                Label {
                    text: row.currency
                    opacity: 0.7
                }
            }

            onClicked: {
                if (page.target === "to")
                    OperationEditor.chooseToAccount(row.accountId);
                else
                    OperationEditor.chooseAccount(row.accountId);
                page.StackView.view.pop();
            }
        }
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - 32
        visible: list.count === 0
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: AccountPickerModel.errorText !== "" ? AccountPickerModel.errorText : qsTr("No active accounts yet.")
    }
}
