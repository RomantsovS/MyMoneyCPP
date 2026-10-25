// The delegates below use ids of this file: "Bound" ties every inline
// component to the scope it is declared in.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The operation form (ADR-0017): an expense, an income or a transfer. The
// amount has the focus when the page opens, the account and the date are
// pre-filled, a recent category is one tap. The state lives in the
// OperationEditor singleton; this page only binds to it.
Page {
    id: page

    // A row "caption · value ▸" that opens a picker.
    component PickerRow: ItemDelegate {
        id: row

        required property string caption
        required property string value

        Layout.fillWidth: true

        contentItem: RowLayout {
            Label {
                text: row.caption
                opacity: 0.7
            }
            Label {
                text: row.value
                elide: Label.ElideRight
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }
            Label {
                text: "▸"
            }
        }
    }

    readonly property bool isTransfer: OperationEditor.type === "transfer"

    title: qsTr("New operation")

    Component.onCompleted: {
        OperationEditor.startNew();
        amountField.forceActiveFocus();
        // Opens the keyboard without a tap. Qt.inputMethod is declared as a
        // plain QObject; at run time it is a QInputMethod, which has show().
        // qmllint disable missing-property
        Qt.inputMethod.show();
        // qmllint enable missing-property
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
                enabled: OperationEditor.valid
                onClicked: {
                    if (OperationEditor.save())
                        page.StackView.view.pop();
                }
            }
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: form.implicitHeight + 32
        clip: true

        ColumnLayout {
            id: form

            x: 16
            y: 16
            width: parent.width - 32
            spacing: 8

            TabBar {
                Layout.fillWidth: true
                currentIndex: ["expense", "income", "transfer"].indexOf(OperationEditor.type)

                TabButton {
                    text: qsTr("Expense")
                    onClicked: OperationEditor.type = "expense"
                }
                TabButton {
                    text: qsTr("Income")
                    onClicked: OperationEditor.type = "income"
                }
                TabButton {
                    text: qsTr("Transfer")
                    onClicked: OperationEditor.type = "transfer"
                }
            }

            RowLayout {
                Layout.fillWidth: true

                TextField {
                    id: amountField

                    Layout.fillWidth: true
                    placeholderText: qsTr("Amount")
                    font.pixelSize: 24
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    text: OperationEditor.amountText
                    onTextEdited: OperationEditor.amountText = text
                }
                Label {
                    text: OperationEditor.currencyCode
                }
            }

            // Only between accounts in different currencies (ADR-0006).
            RowLayout {
                visible: OperationEditor.needsAmountIn
                Layout.fillWidth: true

                TextField {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Amount received")
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    text: OperationEditor.amountInText
                    onTextEdited: OperationEditor.amountInText = text
                }
                Label {
                    text: OperationEditor.toCurrencyCode
                }
            }

            PickerRow {
                visible: !page.isTransfer
                caption: qsTr("Category")
                value: OperationEditor.categoryName !== "" ? OperationEditor.categoryName : qsTr("Choose")
                onClicked: page.StackView.view.push(categoryPicker)
            }

            // The recent categories: the usual case is one tap here.
            Flow {
                visible: !page.isTransfer && OperationEditor.recentCategories.length > 0
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: OperationEditor.recentCategories

                    delegate: Button {
                        // A map {categoryId, name} (OperationEditor).
                        required property var modelData

                        text: modelData.name
                        highlighted: modelData.categoryId === OperationEditor.categoryId
                        onClicked: OperationEditor.chooseCategory(modelData.categoryId)
                    }
                }
            }

            PickerRow {
                caption: page.isTransfer ? qsTr("From") : qsTr("Account")
                value: OperationEditor.accountName !== "" ? OperationEditor.accountName : qsTr("Choose")
                onClicked: page.StackView.view.push(accountPicker, {
                    target: "from"
                })
            }

            PickerRow {
                visible: page.isTransfer
                caption: qsTr("To")
                value: OperationEditor.toAccountName !== "" ? OperationEditor.toAccountName : qsTr("Choose")
                onClicked: page.StackView.view.push(accountPicker, {
                    target: "to"
                })
            }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    text: qsTr("Today")
                    highlighted: OperationEditor.isToday
                    onClicked: OperationEditor.setToday()
                }
                Button {
                    text: qsTr("Yesterday")
                    highlighted: OperationEditor.isYesterday
                    onClicked: OperationEditor.setYesterday()
                }
                Button {
                    readonly property bool otherDay: !OperationEditor.isToday && !OperationEditor.isYesterday

                    text: otherDay ? OperationEditor.dateText : qsTr("Other date")
                    highlighted: otherDay
                    onClicked: dateDialog.open()
                }
            }

            TextField {
                Layout.fillWidth: true
                placeholderText: qsTr("Comment")
                text: OperationEditor.comment
                onTextEdited: OperationEditor.comment = text
            }

            Label {
                visible: OperationEditor.errorText !== ""
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: Material.color(Material.Red)
                text: OperationEditor.errorText
            }
        }
    }

    DateDialog {
        id: dateDialog
    }

    Component {
        id: categoryPicker

        CategoryPicker {}
    }

    Component {
        id: accountPicker

        AccountPicker {}
    }
}
