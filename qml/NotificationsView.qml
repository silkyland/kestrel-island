import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    id: view
    signal backRequested()
    spacing: 8
    RowLayout {
        IslandButton { text: "Back to tools"; symbol: "arrow-left"; implicitWidth: 28; implicitHeight: 28; onClicked: view.backRequested() }
        Text { textFormat: Text.PlainText; text: "Notifications"; color: "#f5f5f7"; font.pixelSize: 20; font.weight: Font.DemiBold }
        Item { Layout.fillWidth: true }
        IslandAction { text: "Clear"; enabled: island.notifications.entries.length > 0; onClicked: island.notifications.clear() }
    }
    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: island.notifications.available ? island.notifications.status : "Privacy validation required"; color: island.notifications.available ? "#a7a7af" : "#ffc078"; font.pixelSize: 13; wrapMode: Text.WordWrap }
    Text { textFormat: Text.PlainText; visible: !island.notifications.available; Layout.fillWidth: true; text: "Plasma still handles your notifications. Collection here is disabled until lock-screen and Do Not Disturb checks pass on this KDE session."; color: "#a7a7af"; font.pixelSize: 12; wrapMode: Text.WordWrap }
    RowLayout {
        Layout.fillWidth: true
        Switch { id: notificationSwitch; checked: island.notifications.enabled; enabled: island.notifications.available; Accessible.name: "Enable Plasma notification watcher"; onToggled: island.configureNotifications(checked,apps.text) }
        Text { textFormat: Text.PlainText; text: "Receive allowed apps"; color: "#a7a7af"; font.pixelSize: 12 }
        Item { Layout.fillWidth: true }
    }
    TextField { id: apps; Layout.fillWidth: true; enabled: island.notifications.available; text: island.notifications.allowlist.join(","); placeholderText: "Allowed app IDs, comma-separated"; Accessible.name: "Allowed notification application IDs"; palette.text: "#f5f5f7"; palette.placeholderText: "#a7a7af"; palette.base: "#242427"; font.pixelSize: 12; onEditingFinished: island.configureNotifications(notificationSwitch.checked,text) }
    Text { textFormat: Text.PlainText; visible: island.notifications.available; Layout.fillWidth: true; text: "Opt-in and memory only. Plasma forwards new notifications to the watcher; only allowed apps are kept. Lock or quiet mode clears the inbox."; color: "#a7a7af"; font.pixelSize: 11; wrapMode: Text.WordWrap }
    ListView {
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: island.notifications.entries
        Text { textFormat: Text.PlainText; anchors.centerIn: parent; visible: island.notifications.entries.length === 0; text: "No notifications collected"; color: "#a7a7af"; font.pixelSize: 12 }
        delegate: Column {
            required property var modelData
            width: ListView.view.width; spacing: 4
            Text { textFormat: Text.PlainText; width: parent.width; text: modelData.app+" · "+modelData.summary; color: "#f5f5f7"; font.pixelSize: 12; elide: Text.ElideRight }
            Text { textFormat: Text.PlainText; width: parent.width; text: modelData.body; color: "#a7a7af"; font.pixelSize: 11; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight }
            Item { width: 1; height: 10 }
        }
    }
}
