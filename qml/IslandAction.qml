import QtQuick
import QtQuick.Controls
Button {
    id: control
    property bool primary: false
    property string symbol: ""
    implicitHeight: 32
    implicitWidth: Math.max(64, row.implicitWidth + 24)
    padding: 8
    opacity: !enabled ? 0.4 : down ? 0.8 : 1
    scale: enabled && hovered ? 1.04 : 1
    Behavior on scale { NumberAnimation { duration: Theme.dur(120); easing.type: Easing.OutCubic } }
    contentItem: Item {
      implicitWidth: row.implicitWidth; implicitHeight: row.implicitHeight
      Row {
        id: row
        anchors.centerIn: parent
        spacing: control.symbol ? 6 : 0
        Image {
            visible: control.symbol.length > 0
            source: visible ? "qrc:/assets/icons/" + control.symbol + ".svg" : ""
            sourceSize.width: 14; sourceSize.height: 14
            width: 14; height: 14
            anchors.verticalCenter: parent.verticalCenter
        }
        Text { textFormat: Text.PlainText; id: label; text: control.text; color: control.primary ? Theme.accentInk : Theme.ink; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
      }
    }
    background: Rectangle { radius: 10; color: control.primary ? (control.down ? Qt.darker(Theme.accent, 1.15) : control.hovered ? Qt.lighter(Theme.accent, 1.12) : Theme.accent) : control.checked ? Theme.selectedFill : control.down ? Theme.raisedPressed : control.hovered ? Theme.raisedHover : Theme.raised; border.width: control.activeFocus ? 2 : control.checked ? 1 : 0; border.color: Theme.tint; Behavior on color { ColorAnimation { duration: Theme.dur(120) } } }
}
