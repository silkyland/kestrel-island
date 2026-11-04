import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string symbol: ""
    property string label: ""
    property bool prominent: false
    property bool selected: false
    property bool dim: false
    readonly property bool showLabel: label.length > 0 && selected
    implicitWidth: showLabel ? row.implicitWidth + 24 : 40
    implicitHeight: 40
    padding: 8
    icon.source: "qrc:/assets/icons/" + symbol + ".svg"
    icon.width: 20
    icon.height: 20
    display: AbstractButton.IconOnly
    Accessible.name: text
    Behavior on implicitWidth { NumberAnimation { duration: island.animations ? 180 : 0; easing.type: Easing.OutCubic } }
    // Render our bundled SVG directly: KDE's platform Button delegate may
    // ignore icon.source when icon.name is empty.
    contentItem: Row {
        id: row
        spacing: 6
        Image {
            objectName: "buttonIcon"
            anchors.verticalCenter: parent.verticalCenter
            source: control.icon.source
            width: 20; height: 20
            sourceSize.width: 20
            sourceSize.height: 20
            fillMode: Image.PreserveAspectFit
            opacity: !control.enabled ? 0.3 : control.dim && !control.selected && !control.hovered ? 0.6 : 1
        }
        Text {
            visible: control.showLabel
            anchors.verticalCenter: parent.verticalCenter
            textFormat: Text.PlainText
            text: control.label
            color: "#f5f5f7"
            font.pixelSize: 12
            font.weight: Font.Medium
        }
    }
    background: Rectangle {
        radius: height / 2
        color: control.down ? "#4a4a53" : control.prominent ? (control.hovered ? "#6e57c2" : "#5a44a8") : control.checked || control.selected ? "#3b324f" : control.hovered || control.activeFocus ? "#2e2e34" : "transparent"
        border.width: control.activeFocus ? 2 : control.checked || control.selected ? 1 : 0
        border.color: "#b8a1ff"
        Behavior on color { ColorAnimation { duration: island.animations ? 120 : 0 } }
    }
    ToolTip.visible: hovered && text.length > 0
    ToolTip.text: text
    ToolTip.delay: 650
}
