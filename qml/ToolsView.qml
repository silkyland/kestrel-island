import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    signal openPage(int page)
    spacing: 8
    Text { textFormat: Text.PlainText; text: "Tools"; color: Theme.ink; font.pixelSize: 20; font.weight: Font.DemiBold }
    Repeater {
        model: [
            {title: "Keep awake", symbol: "coffee", detail: island.awake.snapshot.state === "active" ? "Active · timed session" : "Keep the display and session awake", page: 4},
            {title: "Task activity", symbol: "terminal", detail: island.tasks.enabled ? "Hooks enabled · " + island.tasks.tasks.length + " tasks" : "Optional status from your commands", page: 5},
            {title: "Notification inbox", symbol: "bell", detail: island.notifications.available ? island.notifications.status : "Optional · awaiting KDE privacy validation", page: 6}
        ]
        Rectangle {
            id: card
            required property var modelData
            Layout.fillWidth: true
            implicitHeight: 58
            radius: 16
            color: tap.pressed ? Theme.raisedPressed : hover.hovered ? Theme.raisedHover : Theme.raised
            Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
            HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
            TapHandler { id: tap; onTapped: openPage(card.modelData.page) }
            Accessible.role: Accessible.Button
            Accessible.name: "Open " + modelData.title
            Accessible.onPressAction: openPage(modelData.page)
            RowLayout {
                anchors { fill: parent; leftMargin: 12; rightMargin: 14 }
                spacing: 12
                Rectangle {
                    Layout.preferredWidth: 34; Layout.preferredHeight: 34
                    radius: 11; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.14)
                    Image { anchors.centerIn: parent; width: 18; height: 18; source: "qrc:/assets/icons/" + card.modelData.symbol + ".svg"; sourceSize.width: 18; sourceSize.height: 18 }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 2
                    Text { textFormat: Text.PlainText; text: card.modelData.title; color: Theme.ink; font.pixelSize: 13; font.weight: Font.Medium }
                    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: card.modelData.detail; color: Theme.secondary; font.pixelSize: 11; elide: Text.ElideRight }
                }
                Text { textFormat: Text.PlainText; text: "›"; color: hover.hovered ? Theme.accent : Theme.faint; font.pixelSize: 20 }
            }
        }
    }
    Item { Layout.fillHeight: true }
}
