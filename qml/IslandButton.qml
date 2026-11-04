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
            width: 20; height: 20
            sourceSize.width: 20
            sourceSize.height: 20
            fillMode: Image.PreserveAspectFit
            opacity: control.dim && !control.selected && !control.hovered ? 0.6 : 1
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
        radius: control.prominent ? height / 2 : 10
        color: control.prominent ? (control.down ? Qt.darker(Theme.tintFill, 1.12) : control.hovered ? Theme.tintFillHover : Theme.tintFill)
               : control.down ? Theme.raisedPressed
               : control.checked || control.selected ? Theme.selectedFill
               : control.hovered || control.activeFocus ? Theme.raisedHover
               : "transparent"
        border.width: control.activeFocus ? 2 : control.checked || control.selected ? 1 : 0
        border.color: Theme.tint
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
    }
    ToolTip.visible: hovered && text.length > 0
    ToolTip.text: text
    ToolTip.delay: 650
}
