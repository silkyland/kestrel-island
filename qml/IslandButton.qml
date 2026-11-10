import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string symbol: ""
    property string label: ""
    property int idleWidth: 40
    property int glyphSize: 20
    // Transport buttons sit on a circle; tabs and utilities use the island's
    // rounded-rect plates.
    property bool circular: false
    property bool selected: false
    property bool dim: false
    readonly property bool showLabel: label.length > 0 && selected
    readonly property bool active: checked || selected
    implicitWidth: showLabel ? row.implicitWidth + 24 : idleWidth
    implicitHeight: 40
    padding: 8
    icon.source: "qrc:/assets/icons/" + symbol + ".svg"
    display: AbstractButton.IconOnly
    Accessible.name: text
    opacity: !enabled ? 0.4 : down ? 0.8 : 1
    scale: enabled && hovered ? 1.04 : 1
    Behavior on scale { NumberAnimation { duration: Theme.dur(120); easing.type: Easing.OutCubic } }
    Behavior on implicitWidth { NumberAnimation { duration: Theme.dur(180); easing.type: Easing.OutCubic } }
    // Render our bundled SVG directly: KDE's platform Button delegate may
    // ignore icon.source when icon.name is empty.
    contentItem: Row {
        id: row
        spacing: 6
        Image {
            objectName: "buttonIcon"
            anchors.verticalCenter: parent.verticalCenter
            source: control.icon.source
            width: control.glyphSize; height: control.glyphSize
            sourceSize.width: control.glyphSize
            sourceSize.height: control.glyphSize
            fillMode: Image.PreserveAspectFit
            opacity: control.dim && !control.active && !control.hovered ? 0.6 : 1
        }
        Text {
            visible: control.showLabel
            anchors.verticalCenter: parent.verticalCenter
            textFormat: Text.PlainText
            text: control.label
            color: Theme.ink
            font.pixelSize: 12
            font.weight: Font.Medium
        }
    }
    background: Rectangle {
        radius: control.circular ? height / 2 : 10
        color: control.down ? Theme.raisedPressed
               : control.active ? Theme.selectedFill
               : control.hovered || control.activeFocus ? (control.circular ? Theme.selectedFill : Theme.raisedHover)
               : "transparent"
        // A ring means keyboard focus, never selection.
        border.width: control.activeFocus ? 2 : 0
        border.color: Theme.tint
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
    }
    ToolTip.visible: hovered && text.length > 0
    ToolTip.text: text
    ToolTip.delay: 650
}
