import QtQuick
import QtQuick.Layouts

// Glanceable metric card for the System page grid: title + symbol, a large
// value, a 3 px meter and a subtitle line (device name, free space, folded
// temperature). `attention` turns value and meter to Theme.warn (load >= 85%
// or disk free < 10%). `meter` < 0 hides the meter.
Rectangle {
    id: card
    required property string title
    property string symbol: ""
    property string value: ""
    property string subtitle: ""
    property real meter: -1
    property color meterTint: Theme.accent
    property bool attention: false
    implicitHeight: 84
    radius: 12
    color: Theme.raised
    Accessible.role: Accessible.StaticText
    Accessible.name: title + ": " + value + (subtitle.length ? ", " + subtitle : "")
    ColumnLayout {
        anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 9; bottomMargin: 9 }
        spacing: 3
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Image {
                visible: card.symbol.length > 0
                source: visible ? "qrc:/assets/icons/" + card.symbol + ".svg" : ""
                sourceSize.width: 14; sourceSize.height: 14
                Layout.preferredWidth: 14; Layout.preferredHeight: 14
            }
            Text {
                textFormat: Text.PlainText
                text: card.title
                color: Theme.secondary
                font.pixelSize: 11
                font.weight: Font.Medium
                font.letterSpacing: 0.5
            }
            Item { Layout.fillWidth: true }
        }
        Text {
            textFormat: Text.PlainText
            Layout.fillWidth: true
            text: card.value
            color: card.attention ? Theme.warn : Theme.ink
            font.pixelSize: 20
            font.weight: Font.Medium
            elide: Text.ElideRight
        }
        Rectangle {
            Layout.fillWidth: true
            visible: card.meter >= 0
            height: 3
            radius: 1.5
            color: Theme.track
            Rectangle {
                width: parent.width * Math.max(0, Math.min(100, card.meter)) / 100
                height: parent.height
                radius: 1.5
                color: card.attention ? Theme.warn : card.meterTint
                Behavior on width { NumberAnimation { duration: Theme.dur(200); easing.type: Easing.OutCubic } }
            }
        }
        Text {
            textFormat: Text.PlainText
            Layout.fillWidth: true
            text: card.subtitle
            color: Theme.secondary
            font.pixelSize: 10
            elide: Text.ElideRight
        }
    }
}
