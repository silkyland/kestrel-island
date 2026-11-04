import QtQuick
import QtQuick.Controls

// Dark check control on island tokens, replacing the light platform
// CheckBox/Switch. Keeps CheckBox semantics: Space toggles, checked/toggled,
// and callers set text and Accessible.name.
CheckBox {
    id: control
    implicitHeight: 30
    spacing: 8
    indicator: Rectangle {
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth: 18
        implicitHeight: 18
        radius: 5
        color: !control.enabled ? Theme.raised
             : control.checked ? Theme.accent
             : control.down ? Theme.raisedPressed
             : control.hovered ? Theme.raisedHover
             : Theme.raised
        border.width: control.activeFocus ? 2 : control.checked ? 0 : 1
        border.color: control.activeFocus ? Theme.tint : Theme.divider
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
        Text {
            anchors.centerIn: parent
            text: "✓"
            visible: control.checked
            color: Theme.accentInk
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }
    }
    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        textFormat: Text.PlainText
        color: control.enabled ? Theme.ink : Theme.secondary
        font.pixelSize: 12
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 8
        color: control.down ? Theme.raisedPressed : control.hovered ? Theme.raisedHover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
    }
}
