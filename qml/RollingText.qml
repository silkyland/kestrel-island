import QtQuick
Row {
    id: roll
    property string text: ""
    property color color: "#f5f5f7"
    property int pixelSize: 44
    property int weight: Font.Light
    property bool animate: true
    Accessible.role: Accessible.StaticText
    Accessible.name: text
    Repeater {
        model: roll.text.length
        Item {
            id: cell
            required property int index
            readonly property string value: roll.text.charAt(index)
            property string shown: value
            property string previous: ""
            property real progress: 1
            width: metrics.advanceWidth
            height: metrics.height
            clip: true
            TextMetrics { id: metrics; font: incoming.font; text: cell.value }
            onValueChanged: {
                if (!roll.animate || shown === value) { shown = value; return }
                previous = shown; shown = value
                slide.restart()
            }
            NumberAnimation { id: slide; target: cell; property: "progress"; from: 0; to: 1; duration: 260; easing.type: Easing.OutCubic }
            Text {
                id: outgoing
                textFormat: Text.PlainText
                text: cell.previous
                color: roll.color
                font.pixelSize: roll.pixelSize; font.weight: roll.weight; font.features: {"tnum": 1}
                y: -cell.height * cell.progress
                opacity: 1 - cell.progress
                visible: cell.progress < 1
            }
            Text {
                id: incoming
                textFormat: Text.PlainText
                text: cell.shown
                color: roll.color
                font.pixelSize: roll.pixelSize; font.weight: roll.weight; font.features: {"tnum": 1}
                y: cell.height * (1 - cell.progress)
                opacity: cell.progress
            }
        }
    }
}
