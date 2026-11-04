import QtQuick
import QtQuick.Controls
Button {
    id: control
    property bool primary: false
    property string symbol: ""
    implicitHeight: 32
    implicitWidth: Math.max(64, row.implicitWidth + 24)
    padding: 8
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
            opacity: control.enabled ? 1 : 0.4
        }
        Text { textFormat: Text.PlainText; id: label; text: control.text; color: !control.enabled ? "#777780" : control.primary ? "#16121e" : "#f5f5f7"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
      }
    }
    background: Rectangle { radius: height / 2; color: control.primary ? (control.down ? "#a28ae7" : control.hovered ? "#cbb9ff" : "#b8a1ff") : control.checked ? "#3b324f" : control.down ? "#45454d" : control.hovered ? "#333339" : "#212125"; border.width: control.activeFocus || control.checked ? (control.activeFocus ? 2 : 1) : 0; border.color: "#b8a1ff"; Behavior on color { ColorAnimation { duration: island.animations ? 120 : 0 } } }
}
