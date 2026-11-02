import QtQuick
import QtQuick.Controls

Window {
    id: target
    required property string displayName
    visible: false
    color: "#18000000"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    DropArea {
        id: dropArea
        objectName: "moveDropArea"
        anchors.fill: parent
        keys: ["application/x-kestrel-island"]
        onDropped: function(drop) {
            island.finishMove(target.displayName)
            drop.acceptProposedAction()
        }
    }
    Rectangle {
        anchors.centerIn: parent
        width: 320; height: 112; radius: 24
        color: "#080809"
        border.width: 2
        border.color: dropArea.containsDrag ? "#b8a1ff" : "#55555d"
        Column {
            anchors.centerIn: parent
            spacing: 10
            Text { textFormat: Text.PlainText; anchors.horizontalCenter: parent.horizontalCenter; text: "Move island to " + target.displayName; color: "#f5f5f7"; font.pixelSize: 18; font.weight: Font.DemiBold }
            Text { textFormat: Text.PlainText; anchors.horizontalCenter: parent.horizontalCenter; text: "Release anywhere on this display · Esc to cancel"; color: "#a7a7af"; font.pixelSize: 12 }
        }
    }
}
