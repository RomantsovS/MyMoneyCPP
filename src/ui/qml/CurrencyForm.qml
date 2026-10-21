import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The currency form (ADR-0016): ISO 4217 code and decimal places. The state
// lives in the CurrencyEditor singleton.
Page {
    id: page

    title: qsTr("New currency")

    Component.onCompleted: CurrencyEditor.startNew()

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
                enabled: CurrencyEditor.valid
                onClicked: {
                    if (CurrencyEditor.save())
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
            placeholderText: qsTr("Code, e.g. EUR")
            maximumLength: 3
            inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText
            text: CurrencyEditor.code
            onTextEdited: CurrencyEditor.code = text
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("Decimal places")
                Layout.fillWidth: true
            }
            SpinBox {
                from: 0
                to: 4
                value: CurrencyEditor.minorDigits
                onValueModified: CurrencyEditor.minorDigits = value
            }
        }

        Label {
            visible: CurrencyEditor.errorText !== ""
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Material.color(Material.Red)
            text: CurrencyEditor.errorText
        }
    }
}
