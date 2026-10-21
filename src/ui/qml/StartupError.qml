import QtQuick
import QtQuick.Controls.Material

// Shown instead of Main when the database cannot be opened (ADR-0016), e.g.
// when it was written by a newer version of the app.
ApplicationWindow {
    id: root

    // Passed in from C++ (src/app/main.cpp) with setInitialProperties().
    required property string errorText

    width: 400
    height: 850
    visible: true
    title: qsTr("MyMoney")

    Material.theme: Material.System

    Label {
        anchors.centerIn: parent
        width: parent.width - 32
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: qsTr("The data could not be opened:\n%1").arg(root.errorText)
    }
}
