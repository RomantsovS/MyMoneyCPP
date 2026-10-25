// The delegate below uses ids of this file: "Bound" ties every inline
// component to the scope it is declared in.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The Categories page (ADR-0018): the groups and categories of one kind as a
// tree. Tap a row to rename it; the buttons below add a group or a category.
Page {
    id: page

    title: qsTr("Categories")

    Component.onCompleted: CategoryTreeModel.reload()

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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            Layout.fillWidth: true
            currentIndex: CategoryTreeModel.kind === "income" ? 1 : 0

            TabButton {
                text: qsTr("Expense")
                onClicked: CategoryTreeModel.kind = "expense"
            }
            TabButton {
                text: qsTr("Income")
                onClicked: CategoryTreeModel.kind = "income"
            }
        }

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: CategoryTreeModel

            delegate: ItemDelegate {
                id: row

                // Filled from the model's roles (CategoryTreeModel).
                required property double categoryId
                required property string name
                required property bool isGroup
                required property bool indented

                width: ListView.view.width
                leftPadding: row.indented ? 40 : 16
                font.bold: row.isGroup
                text: row.name
                onClicked: page.StackView.view.push(categoryForm, {
                    categoryId: row.categoryId
                })
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 32
                visible: list.count === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: CategoryTreeModel.errorText !== "" ? CategoryTreeModel.errorText : qsTr("No categories yet.")
            }
        }
    }

    footer: Pane {
        RowLayout {
            anchors.fill: parent

            Button {
                Layout.fillWidth: true
                text: qsTr("+ Group")
                onClicked: page.StackView.view.push(categoryForm, {
                    newGroup: true,
                    kindText: CategoryTreeModel.kind
                })
            }
            Button {
                Layout.fillWidth: true
                highlighted: true
                text: qsTr("+ Category")
                onClicked: page.StackView.view.push(categoryForm, {
                    kindText: CategoryTreeModel.kind
                })
            }
        }
    }

    Component {
        id: categoryForm

        CategoryForm {}
    }
}
