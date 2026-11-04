import QtQuick
import QtQuick.Controls

// Dark bounded stepper on island tokens, replacing the light platform
// SpinBox. Keeps SpinBox semantics: from/to bounds are strictly enforced,
// Up/Down keys step the value when focused, valueModified() fires only on
// user interaction. Set Accessible.name at each call site.
SpinBox {
    id: control
    editable: false
    implicitWidth: 118
    implicitHeight: 32
    padding: 0
    contentItem: Text {
        text: control.displayText
        textFormat: Text.PlainText
        color: control.enabled ? Theme.ink : Theme.secondary
        font.pixelSize: 13
        font.features: {"tnum": 1}
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 10
        color: control.down.pressed || control.up.pressed ? Theme.raisedPressed
             : control.hovered || control.activeFocus ? Theme.raisedHover
             : Theme.raised
        border.width: control.activeFocus ? 2 : 0
        border.color: Theme.tint
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
    }
    up.indicator: Rectangle {
        x: control.width - width - 3
        y: 3
        width: 28
        height: control.height - 6
        radius: 8
        color: control.up.pressed ? Theme.raisedPressed : control.up.hovered ? Theme.raisedHover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
        Text {
            anchors.centerIn: parent
            text: "+"
            color: control.value < control.to ? Theme.ink : Theme.faint
            font.pixelSize: 14
        }
    }
    down.indicator: Rectangle {
        x: 3
        y: 3
        width: 28
        height: control.height - 6
        radius: 8
        color: control.down.pressed ? Theme.raisedPressed : control.down.hovered ? Theme.raisedHover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
        Text {
            anchors.centerIn: parent
            text: "−"
            color: control.value > control.from ? Theme.ink : Theme.faint
            font.pixelSize: 14
        }
    }
}
