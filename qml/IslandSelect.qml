import QtQuick
import QtQuick.Controls

// Dark face for the display ComboBox: platform popup behaviour is kept,
// only the closed face, delegate and popup background follow island tokens.
ComboBox {
    id: control
    implicitHeight: 32
    padding: 0
    leftPadding: 12
    indicator: Text {
        x: control.width - width - 12
        y: (control.height - height) / 2
        text: "▾"
        color: Theme.secondary
        font.pixelSize: 11
    }
    contentItem: Text {
        leftPadding: control.leftPadding
        rightPadding: control.indicator.width + 16
        text: control.displayText
        textFormat: Text.PlainText
        color: control.enabled ? Theme.ink : Theme.secondary
        font.pixelSize: 12
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        // KDE's ComboBox template wraps the face in a MobileCursor that
        // calls text-field methods; stub them on this read-only label.
        readonly property int selectionStart: 0
        function positionToRectangle(pos) { return Qt.rect(0, 0, 0, 0) }
    }
    background: Rectangle {
        radius: 10
        color: control.down ? Theme.raisedPressed
             : control.hovered || control.activeFocus ? Theme.raisedHover
             : Theme.raised
        border.width: control.activeFocus ? 2 : 0
        border.color: Theme.tint
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
    }
    delegate: ItemDelegate {
        id: option
        required property int index
        required property var modelData
        width: control.width
        highlighted: control.highlightedIndex === index
        text: control.textRole ? String(modelData[control.textRole] ?? "") : String(modelData ?? "")
        contentItem: Text {
            text: option.text
            textFormat: Text.PlainText
            color: Theme.ink
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 8
            color: option.highlighted ? Theme.selectedFill : "transparent"
        }
    }
    popup: Popup {
        y: control.height + 4
        width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 6 * 36)
        padding: 4
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
        }
        background: Rectangle {
            radius: 10
            color: Theme.raised
            border.width: 1
            border.color: Theme.divider
        }
    }
}
