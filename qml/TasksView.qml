import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    id: view
    signal backRequested()
    spacing: 8
    RowLayout {
        IslandButton { text: "Back to tools"; symbol: "arrow-left"; implicitWidth: 28; implicitHeight: 28; onClicked: view.backRequested() }
        Text { textFormat: Text.PlainText; text: "Task activity"; color: Theme.ink; font.pixelSize: 20; font.weight: Font.DemiBold }
        Item { Layout.fillWidth: true }
        IslandCheck { checked: island.tasks.enabled; Accessible.name: "Enable task hooks"; onToggled: island.setTasksEnabled(checked) }
    }
    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: island.tasks.enabled ? "Listening for command hooks" : "Show progress from commands you choose."; color: island.tasks.enabled ? Theme.ok : Theme.ink; font.pixelSize: 13 }
    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: "Enable the switch, then run this in your project. The hook reports status; prompts and output stay in your terminal."; color: Theme.secondary; font.pixelSize: 12; wrapMode: Text.WordWrap }
    Rectangle {
        Layout.fillWidth: true; implicitHeight: commandText.implicitHeight + 20; color: Theme.raised; radius: 8
        Text { id: commandText; textFormat: Text.PlainText; anchors { fill: parent; margins: 10 } text: 'kestrel-task --label "Build" -- cmake --build build -j 4'; color: Theme.accent; font.family: "monospace"; font.pixelSize: 11; wrapMode: Text.WrapAnywhere }
    }
    IslandAction { text: "Copy command"; onClicked: island.copyTaskExample() }
    Item {
        Layout.fillWidth: true; Layout.fillHeight: true
        ListView {
            id: taskList
            anchors.fill: parent
            clip: true
            model: island.tasks.tasks
            Text { textFormat: Text.PlainText; anchors.centerIn: parent; visible: island.tasks.tasks.length === 0; text: island.tasks.enabled ? "Ready · no tasks yet" : "Hooks are off"; color: Theme.secondary; font.pixelSize: 12 }
            delegate: RowLayout {
                required property var modelData
                width: ListView.view.width; height: 52
                ColumnLayout { Layout.fillWidth: true; Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.label; color: Theme.ink; font.pixelSize: 12; elide: Text.ElideRight } Text { textFormat: Text.PlainText; text: modelData.state+" · "+modelData.elapsed+"s"; color: modelData.state === "failed" ? Theme.timer : Theme.accent; font.pixelSize: 11 } }
                IslandButton { text: "Dismiss task"; symbol: "process-stop"; onClicked: island.tasks.dismiss(modelData.id) }
            }
        }
        EdgeFade { anchors.fill: parent; target: taskList }
    }
}
