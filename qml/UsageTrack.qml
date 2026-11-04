import QtQuick
Rectangle {
    property real value: 0
    property color tint: Theme.accent
    height: 3
    radius: 1.5
    color: Theme.track
    Rectangle { width: parent.width * Math.max(0, Math.min(100, parent.value)) / 100; height: parent.height; radius: 1.5; color: parent.tint }
}
