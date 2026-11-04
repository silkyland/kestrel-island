import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    id: view
    signal backRequested()
    spacing: 8
    RowLayout {
        IslandButton { text: "Back to tools"; symbol: "arrow-left"; implicitWidth: 28; implicitHeight: 28; onClicked: view.backRequested() }
        Text { textFormat: Text.PlainText; text: "Notifications"; color: Theme.ink; font.pixelSize: 20; font.weight: Font.DemiBold }
        Item { Layout.fillWidth: true }
        IslandAction { text: "Clear"; enabled: island.notifications.entries.length > 0; onClicked: island.notifications.clear() }
    }
    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: island.notifications.available ? island.notifications.status : "Privacy validation required"; color: island.notifications.available ? Theme.secondary : Theme.timer; font.pixelSize: 13; wrapMode: Text.WordWrap }
    Text { textFormat: Text.PlainText; visible: !island.notifications.available; Layout.fillWidth: true; text: "Plasma still handles your notifications. Collection here is disabled until lock-screen and Do Not Disturb checks pass on this KDE session."; color: Theme.secondary; font.pixelSize: 12; wrapMode: Text.WordWrap }
    RowLayout {
        Layout.fillWidth: true
        IslandCheck { id: notificationSwitch; checked: island.notifications.enabled; enabled: island.notifications.available; Accessible.name: "Enable Plasma notification watcher"; onToggled: island.configureNotifications(checked,apps.text) }
        Text { textFormat: Text.PlainText; text: "Receive allowed apps"; color: Theme.secondary; font.pixelSize: 12 }
        Item { Layout.fillWidth: true }
    }
    TextField {
        id: apps
        Layout.fillWidth: true
        enabled: island.notifications.available
        text: island.notifications.allowlist.join(",")
        placeholderText: "Allowed app IDs, comma-separated"
        Accessible.name: "Allowed notification application IDs"
        font.pixelSize: 12
        color: Theme.ink
        placeholderTextColor: Theme.secondary
        onEditingFinished: island.configureNotifications(notificationSwitch.checked,text)
        background: Rectangle {
            radius: 10
            color: apps.enabled ? Theme.raised : Theme.surface
            border.width: apps.activeFocus ? 2 : 1
            border.color: apps.activeFocus ? Theme.tint : Theme.divider
        }
    }
    Text { textFormat: Text.PlainText; visible: island.notifications.available; Layout.fillWidth: true; text: "Opt-in and memory only. Plasma forwards new notifications to the watcher; only allowed apps are kept. Lock or quiet mode clears the inbox."; color: Theme.secondary; font.pixelSize: 11; wrapMode: Text.WordWrap }
    Item {
        Layout.fillWidth: true; Layout.fillHeight: true
        ListView {
            id: inboxList
            anchors.fill: parent
            clip: true
            model: island.notifications.entries
            Text { textFormat: Text.PlainText; anchors.centerIn: parent; visible: island.notifications.entries.length === 0; text: "No notifications collected"; color: Theme.secondary; font.pixelSize: 12 }
            delegate: Column {
                required property var modelData
                width: ListView.view.width; spacing: 4
                Text { textFormat: Text.PlainText; width: parent.width; text: modelData.app+" · "+modelData.summary; color: Theme.ink; font.pixelSize: 12; elide: Text.ElideRight }
                Text { textFormat: Text.PlainText; width: parent.width; text: modelData.body; color: Theme.secondary; font.pixelSize: 11; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight }
                Item { width: 1; height: 10 }
            }
        }
        EdgeFade { anchors.fill: parent; target: inboxList }
    }
}
