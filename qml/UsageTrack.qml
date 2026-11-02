import QtQuick
Rectangle {
    property real value: 0
    property color tint: "#b8a1ff"
    height: 3
    radius: 1.5
    color: "#29292e"
    Rectangle { width: parent.width * Math.max(0, Math.min(100, parent.value)) / 100; height: parent.height; radius: 1.5; color: parent.tint }
}
