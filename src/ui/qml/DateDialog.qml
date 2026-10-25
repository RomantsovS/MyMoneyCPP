// The delegate below uses ids of this file: "Bound" ties every inline
// component to the scope it is declared in.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import MyMoney.UI

// The calendar for any date other than today or yesterday (ADR-0017). It
// passes the chosen date to OperationEditor as numbers - never as a
// JavaScript Date, which is a time in a time zone.
Dialog {
    id: dialog

    // The month on screen, as MonthGrid counts it: month 0..11.
    property int shownYear: OperationEditor.year
    property int shownMonth: OperationEditor.month - 1

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 32, 400)
    modal: true
    title: qsTr("Date")
    standardButtons: Dialog.Cancel

    // Always start at the month of the current date.
    onAboutToShow: {
        shownYear = OperationEditor.year;
        shownMonth = OperationEditor.month - 1;
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true

            ToolButton {
                text: "‹"
                onClicked: {
                    if (dialog.shownMonth === 0) {
                        dialog.shownMonth = 11;
                        dialog.shownYear -= 1;
                    } else {
                        dialog.shownMonth -= 1;
                    }
                }
            }
            Label {
                text: grid.title
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            ToolButton {
                text: "›"
                onClicked: {
                    if (dialog.shownMonth === 11) {
                        dialog.shownMonth = 0;
                        dialog.shownYear += 1;
                    } else {
                        dialog.shownMonth += 1;
                    }
                }
            }
        }

        DayOfWeekRow {
            locale: grid.locale
            Layout.fillWidth: true
        }

        MonthGrid {
            id: grid

            month: dialog.shownMonth
            year: dialog.shownYear
            locale: Qt.locale()
            Layout.fillWidth: true

            delegate: Label {
                id: cell

                // Filled from MonthGrid's model (month 0..11).
                required property int day
                required property int month
                required property int year
                required property bool today

                readonly property bool chosen: cell.year === OperationEditor.year && cell.month + 1 === OperationEditor.month && cell.day === OperationEditor.day

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                opacity: cell.month === grid.month ? 1 : 0.4
                font.bold: cell.today || cell.chosen
                color: cell.chosen ? Material.accent : Material.foreground
                text: cell.day

                TapHandler {
                    onTapped: {
                        OperationEditor.setDate(cell.year, cell.month + 1, cell.day);
                        dialog.close();
                    }
                }
            }
        }
    }
}
