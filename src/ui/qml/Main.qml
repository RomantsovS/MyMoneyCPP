import QtQuick

// M0: an empty window that proves the whole chain works:
// C++ core -> Qt app -> QML, on desktop and Android.
// The visual style (D10) and navigation (D11) are decided in M4.
Window {
    id: root

    // Passed in from C++ (src/app/main.cpp) with setInitialProperties().
    required property string appVersion

    width: 400
    height: 850
    visible: true
    title: qsTr("MyMoney")

    Text {
        anchors.centerIn: parent
        text: qsTr("MyMoney %1").arg(root.appVersion)
        font.pixelSize: 24
    }
}
