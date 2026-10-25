import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The journal's filters (ADR-0019): an account, a category or group, and a
// comment text. The text searches the whole history, so it is applied on
// Enter or when the dialog closes, not on every keystroke.
Dialog {
    id: dialog

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 32, 400)
    // With the focus, Back closes the dialog (Popup.CloseOnEscape) instead
    // of reaching the page below it. "Release" outside, not "press": the back
    // gesture starts with a touch at the screen edge that Android then takes
    // back - on press it would close the dialog, and the back key would then
    // act on the page as well.
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnReleaseOutside
    modal: true
    title: qsTr("Filter")
    standardButtons: Dialog.Close

    onAboutToShow: commentField.text = JournalModel.commentText
    onClosed: JournalModel.commentText = commentField.text

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: qsTr("Account")
        }
        ComboBox {
            Layout.fillWidth: true
            model: JournalModel.accountChoices
            textRole: "name"
            valueRole: "id"
            currentIndex: indexOfValue(JournalModel.accountId)
            onActivated: JournalModel.accountId = currentValue
        }

        Label {
            text: qsTr("Category or group")
        }
        ComboBox {
            Layout.fillWidth: true
            model: JournalModel.categoryChoices
            textRole: "name"
            valueRole: "id"
            currentIndex: indexOfValue(JournalModel.categoryId)
            onActivated: JournalModel.categoryId = currentValue
        }

        Label {
            text: qsTr("Comment contains (all months)")
        }
        TextField {
            id: commentField

            Layout.fillWidth: true
            placeholderText: qsTr("Text")
            onAccepted: JournalModel.commentText = text
        }
    }
}
