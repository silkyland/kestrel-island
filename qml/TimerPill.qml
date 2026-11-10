import QtQuick
import QtQuick.Controls

// The timer's one coloured action: amber text on an 18% amber capsule, as
// the island's Start and Pause buttons. Not a filled primary button: the
// ruler's colour stays the loudest thing on the page.
Button {
    id: pill
    implicitHeight: 28
    implicitWidth: Math.max(56, contentItem.implicitWidth + leftPadding + rightPadding)
    leftPadding: 14
    rightPadding: 14
    opacity: !enabled ? 0.4 : down ? 0.8 : 1
    Accessible.name: text
    contentItem: Text {
        text: pill.text
        color: Theme.timer
        font.pixelSize: 12
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: height / 2
        color: Qt.rgba(Theme.timer.r, Theme.timer.g, Theme.timer.b, pill.down ? 0.26 : pill.hovered ? 0.22 : 0.18)
        border.width: pill.activeFocus ? 1 : 0
        border.color: Theme.timer
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
    }
}
