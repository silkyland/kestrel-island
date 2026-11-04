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
        color: Theme.surface
        border.width: 2
        border.color: dropArea.containsDrag ? Theme.accent : Theme.faint
        Column {
            anchors.centerIn: parent
            spacing: 10
            Text { textFormat: Text.PlainText; anchors.horizontalCenter: parent.horizontalCenter; text: "Move island to " + target.displayName; color: Theme.ink; font.pixelSize: 18; font.weight: Font.DemiBold }
            Text { textFormat: Text.PlainText; anchors.horizontalCenter: parent.horizontalCenter; text: "Release anywhere on this display · Esc to cancel"; color: Theme.secondary; font.pixelSize: 12 }
        }
    }
}
