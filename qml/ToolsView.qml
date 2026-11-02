import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    signal openPage(int page)
    spacing: 8
    Text { textFormat: Text.PlainText; text: "Island tools"; color: "#f5f5f7"; font.pixelSize: 18; font.weight: Font.DemiBold }
    Repeater {
        model: [
            {title: "Keep awake", symbol: "coffee", detail: island.awake.snapshot.state === "active" ? "Active · timed session" : "Keep the display and session awake", page: 4},
            {title: "Task activity", symbol: "terminal", detail: island.tasks.enabled ? "Hooks enabled · " + island.tasks.tasks.length + " tasks" : "Optional status from your commands", page: 5},
            {title: "Notification inbox", symbol: "bell", detail: island.notifications.available ? island.notifications.status : "Optional · awaiting KDE privacy validation", page: 6}
        ]
        RowLayout {
            required property var modelData
            Layout.fillWidth: true; spacing: 12
            Rectangle {
                Layout.preferredWidth: 34; Layout.preferredHeight: 34
                radius: 10; color: "#202023"
                Image { anchors.centerIn: parent; width: 18; height: 18; source: "qrc:/assets/icons/" + modelData.symbol + ".svg"; sourceSize.width: 18; sourceSize.height: 18 }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                Text { textFormat: Text.PlainText; text: modelData.title; color: "#f5f5f7"; font.pixelSize: 13; font.weight: Font.Medium }
                Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.detail; color: "#a7a7af"; font.pixelSize: 11; wrapMode: Text.WordWrap }
            }
            IslandAction { text: "Open"; Accessible.name: "Open " + modelData.title; onClicked: openPage(modelData.page) }
        }
    }
    Item { Layout.fillHeight: true }
}
