import QtQuick
import QtQuick.Controls

// One word of a segmented mode switch: the selected word is white with an
// amber underline, the rest sit at half strength. No plate of its own.
Button {
    id: tab
    property bool selected: false
    checkable: true
    Binding { target: tab; property: "checked"; value: tab.selected }
    implicitHeight: 28
    implicitWidth: contentItem.implicitWidth
    padding: 0
    Accessible.name: text + " mode"
    contentItem: Item {
        implicitWidth: label.implicitWidth + 8
        implicitHeight: 28
        Text {
            id: label
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            textFormat: Text.PlainText
            text: tab.text
            color: tab.selected ? Theme.ink : Qt.rgba(1, 1, 1, 0.5)
            font.pixelSize: 13
            font.weight: Font.Medium
        }
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            width: label.implicitWidth + 8
            height: 2
            radius: 1
            color: Theme.timer
            visible: tab.selected
        }
    }
    background: Item {}
}
