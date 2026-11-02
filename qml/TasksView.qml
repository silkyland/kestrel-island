import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    id: view
    signal backRequested()
    spacing: 8
    RowLayout {
        IslandButton { text: "Back to tools"; symbol: "arrow-left"; implicitWidth: 28; implicitHeight: 28; onClicked: view.backRequested() }
        Text { textFormat: Text.PlainText; text: "Task activity"; color: "#f5f5f7"; font.pixelSize: 20; font.weight: Font.DemiBold }
        Item { Layout.fillWidth: true }
        Switch { checked: island.tasks.enabled; Accessible.name: "Enable task hooks"; onToggled: island.setTasksEnabled(checked) }
    }
    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: island.tasks.enabled ? "Listening for command hooks" : "Show progress from commands you choose."; color: island.tasks.enabled ? "#9dd7b1" : "#f5f5f7"; font.pixelSize: 13 }
    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: "Enable the switch, then run this in your project. The hook reports status; prompts and output stay in your terminal."; color: "#a7a7af"; font.pixelSize: 12; wrapMode: Text.WordWrap }
    Rectangle {
        Layout.fillWidth: true; implicitHeight: commandText.implicitHeight + 20; color: "#19191d"; radius: 8
        Text { id: commandText; textFormat: Text.PlainText; anchors { fill: parent; margins: 10 } text: 'kestrel-task --label "Build" -- cmake --build build -j 4'; color: "#c8b7ff"; font.family: "monospace"; font.pixelSize: 11; wrapMode: Text.WrapAnywhere }
    }
    IslandAction { text: "Copy command"; onClicked: island.copyTaskExample() }
    ListView {
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
        model: island.tasks.tasks
        Text { textFormat: Text.PlainText; anchors.centerIn: parent; visible: island.tasks.tasks.length === 0; text: island.tasks.enabled ? "Ready · no tasks yet" : "Hooks are off"; color: "#a7a7af"; font.pixelSize: 12 }
        delegate: RowLayout {
            required property var modelData
            width: ListView.view.width; height: 52
            ColumnLayout { Layout.fillWidth: true; Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.label; color: "#f5f5f7"; font.pixelSize: 12; elide: Text.ElideRight } Text { textFormat: Text.PlainText; text: modelData.state+" · "+modelData.elapsed+"s"; color: modelData.state === "failed" ? "#ffc078" : "#b8a1ff"; font.pixelSize: 11 } }
            IslandButton { text: "Dismiss task"; symbol: "process-stop"; onClicked: island.tasks.dismiss(modelData.id) }
        }
    }
}
