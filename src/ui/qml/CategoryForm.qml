import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The category form (ADR-0018): a new group, a new category (with an optional
// group), or renaming an existing one. Opened from the Categories page and
// from the category picker of the operation form. The state lives in the
// CategoryEditor singleton; this page only binds to it.
Page {
    id: page

    // Set by the page that pushes this one.
    // -1: something new; otherwise the id of the category or group to rename.
    property double categoryId: -1
    // For something new: a group instead of a category, and its kind.
    property bool newGroup: false
    property string kindText: "expense"
    // From the operation form: after saving, choose the new category for the
    // operation and go back to this page (skipping the picker).
    property Item returnTo: null

    title: CategoryEditor.isNew ? (CategoryEditor.isGroup ? qsTr("New group") : qsTr("New category")) : (CategoryEditor.isGroup ? qsTr("Group") : qsTr("Category"))

    Component.onCompleted: {
        if (page.categoryId >= 0)
            CategoryEditor.load(page.categoryId);
        else if (page.newGroup)
            CategoryEditor.startNewGroup(page.kindText);
        else
            CategoryEditor.startNewCategory(page.kindText);
        nameField.forceActiveFocus();
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
                enabled: CategoryEditor.valid
                onClicked: {
                    if (!CategoryEditor.save())
                        return;
                    if (page.returnTo !== null) {
                        OperationEditor.chooseCategory(CategoryEditor.savedId);
                        page.StackView.view.pop(page.returnTo);
                    } else {
                        page.StackView.view.pop();
                    }
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

        Label {
            opacity: 0.7
            text: CategoryEditor.summary
        }

        TextField {
            id: nameField

            Layout.fillWidth: true
            placeholderText: qsTr("Name")
            text: CategoryEditor.name
            onTextEdited: CategoryEditor.name = text
        }

        // Only for a new category: an existing one stays in its group.
        ComboBox {
            visible: CategoryEditor.isNew && !CategoryEditor.isGroup
            Layout.fillWidth: true
            model: CategoryEditor.groups
            textRole: "name"
            valueRole: "categoryId"
            currentIndex: indexOfValue(CategoryEditor.groupId)
            onActivated: CategoryEditor.groupId = currentValue
        }

        Label {
            visible: CategoryEditor.errorText !== ""
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Material.color(Material.Red)
            text: CategoryEditor.errorText
        }
    }
}
