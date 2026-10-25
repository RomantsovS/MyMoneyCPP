// The delegates below use ids of this file: "Bound" ties every inline
// component to the scope it is declared in.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The Journal tab (ADR-0019): the operations of one month, newest first and
// grouped by day; or, while a comment text is set, the search results of the
// whole history. Tap an operation to edit or delete it.
Page {
    id: page

    title: qsTr("Journal")

    Component.onCompleted: JournalModel.reload()

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
                text: qsTr("Filter")
                onClicked: filterDialog.open()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            visible: !JournalModel.searching
            Layout.fillWidth: true

            ToolButton {
                text: "‹"
                font.pixelSize: 24
                onClicked: JournalModel.previousMonth()
            }
            Label {
                text: JournalModel.monthTitle
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            ToolButton {
                text: "›"
                font.pixelSize: 24
                onClicked: JournalModel.nextMonth()
            }
        }

        Label {
            visible: JournalModel.searching
            Layout.fillWidth: true
            padding: 12
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("Comments containing \"%1\", all months").arg(JournalModel.commentText.trim())
        }

        // The active filters; a tap removes one.
        Flow {
            visible: JournalModel.accountId >= 0 || JournalModel.categoryId >= 0 || JournalModel.searching
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            spacing: 8

            Button {
                visible: JournalModel.accountId >= 0
                text: JournalModel.accountName + "  ✕"
                onClicked: JournalModel.accountId = -1
            }
            Button {
                visible: JournalModel.categoryId >= 0
                text: JournalModel.categoryName + "  ✕"
                onClicked: JournalModel.categoryId = -1
            }
            Button {
                visible: JournalModel.searching
                text: "\"" + JournalModel.commentText.trim() + "\"  ✕"
                onClicked: JournalModel.commentText = ""
            }
        }

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: JournalModel

            section.property: "dayText"
            section.criteria: ViewSection.FullString
            section.delegate: Label {
                required property string section

                width: ListView.view.width
                leftPadding: 16
                topPadding: 12
                bottomPadding: 4
                font.bold: true
                color: Material.accent
                text: section
            }

            delegate: ItemDelegate {
                id: row

                // Filled from the model's roles (JournalModel).
                required property double operationId
                required property string title
                required property string account
                required property string amountText
                required property string kind
                required property string comment

                width: ListView.view.width

                contentItem: ColumnLayout {
                    spacing: 2

                    RowLayout {
                        Label {
                            text: row.title
                            elide: Label.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: row.amountText
                            color: row.kind === "expense" ? Material.color(Material.Red) : row.kind === "income" ? Material.color(Material.Green) : Material.foreground
                        }
                    }
                    Label {
                        visible: text !== ""
                        text: [row.account, row.comment].filter(part => part !== "").join(" · ")
                        opacity: 0.7
                        elide: Label.ElideRight
                        Layout.fillWidth: true
                    }
                }

                onClicked: page.StackView.view.push(operationForm, {
                    operationId: row.operationId
                })
            }

            footer: Label {
                width: ListView.view.width
                height: visible ? implicitHeight : 0
                visible: JournalModel.truncated
                padding: 16
                wrapMode: Text.Wrap
                opacity: 0.7
                text: qsTr("Only the newest %1 matches are shown.").arg(JournalModel.searchLimit)
            }
        }
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - 32
        visible: list.count === 0
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: JournalModel.errorText !== "" ? JournalModel.errorText : qsTr("No operations.")
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
        onClicked: page.StackView.view.push(operationForm, {
            operationId: -1
        })
    }

    JournalFilter {
        id: filterDialog
    }

    Component {
        id: operationForm

        OperationForm {}
    }
}
