// The delegates below use ids of this file: "Bound" ties every inline
// component to the scope it is declared in.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The category picker (ADR-0017): the categories of the operation's kind,
// grouped by group. Groups are only section headers - they cannot be chosen
// (ADR-0015).
Page {
    id: page

    title: qsTr("Category")

    Component.onCompleted: CategoryPickerModel.load(OperationEditor.type)

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

        TextField {
            Layout.fillWidth: true
            Layout.margins: 16
            placeholderText: qsTr("Filter")
            text: CategoryPickerModel.filterText
            onTextEdited: CategoryPickerModel.filterText = text
        }

        // A missing category is added without leaving the operation
        // (ADR-0018). After saving, the form returns to the page below this
        // picker - the operation form - with the new category chosen.
        ItemDelegate {
            Layout.fillWidth: true
            text: qsTr("+ New category")
            onClicked: page.StackView.view.push(categoryForm, {
                kindText: OperationEditor.type,
                returnTo: page.StackView.view.get(page.StackView.index - 1)
            })
        }

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: CategoryPickerModel

            section.property: "groupName"
            section.criteria: ViewSection.FullString
            // Categories at the root have no group: no header for them.
            section.delegate: Label {
                required property string section

                width: ListView.view.width
                height: section === "" ? 0 : implicitHeight
                visible: section !== ""
                leftPadding: 16
                topPadding: 12
                bottomPadding: 4
                font.bold: true
                color: Material.accent
                text: section
            }

            delegate: ItemDelegate {
                id: row

                // Filled from the model's roles (CategoryPickerModel).
                required property double categoryId
                required property string name
                required property string groupName

                width: ListView.view.width
                leftPadding: row.groupName === "" ? 16 : 32
                text: row.name
                onClicked: {
                    OperationEditor.chooseCategory(row.categoryId);
                    page.StackView.view.pop();
                }
            }
        }
    }

    Component {
        id: categoryForm

        CategoryForm {}
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - 32
        visible: list.count === 0
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: CategoryPickerModel.errorText !== "" ? CategoryPickerModel.errorText : qsTr("No categories yet. Tap \"+ New category\".")
    }
}
