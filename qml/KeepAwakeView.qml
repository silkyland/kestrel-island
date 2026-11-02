import QtQuick
import QtQuick.Layouts
ColumnLayout {
    id: view
    signal backRequested()
    spacing: 8
    readonly property var awakeState: island.awake.snapshot
    readonly property bool active: awakeState.state === "active"
    readonly property bool busy: awakeState.state === "requesting" || awakeState.state === "releasing"
    RowLayout {
        IslandButton { text: "Back to tools"; symbol: "arrow-left"; implicitWidth: 28; implicitHeight: 28; onClicked: view.backRequested() }
        Text { textFormat: Text.PlainText; text: "Keep awake"; color: "#f5f5f7"; font.pixelSize: 20; font.weight: Font.DemiBold }
        Item { Layout.fillWidth: true }
        Text { textFormat: Text.PlainText; text: view.active ? "On" : view.busy ? "Connecting…" : "Off"; color: view.active ? "#9dd7b1" : "#a7a7af"; font.pixelSize: 12 }
    }
    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: view.active ? (awakeState.timed ? Math.floor(awakeState.remaining/60).toString().padStart(2,"0")+":"+(awakeState.remaining%60).toString().padStart(2,"0") : "Until you stop") : "Stay awake for a while."; color: view.active ? "#9dd7b1" : "#f5f5f7"; font.pixelSize: view.active && awakeState.timed ? 36 : 20; font.weight: Font.Medium }
    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: view.active ? "Display and automatic sleep inhibition confirmed." : "Temporarily keep the display and session awake. Choose how long."; color: "#a7a7af"; font.pixelSize: 12; wrapMode: Text.WordWrap }
    RowLayout {
        visible: !view.active; enabled: !view.busy
        Repeater { model: [15,30,60]; IslandAction { required property int modelData; text: modelData+" min"; onClicked: island.awake.start(modelData) } }
        IslandAction { text: "Until off"; onClicked: island.awake.start(0) }
    }
    Item { Layout.fillHeight: true }
    IslandAction { Layout.fillWidth: true; visible: view.active || view.busy; primary: view.active; text: view.active ? "Stop keeping awake" : "Cancel request"; onClicked: island.awake.stop() }
    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: awakeState.error || "Starts off each launch. Lid and power policies still apply."; color: awakeState.error ? "#ffc078" : "#a7a7af"; font.pixelSize: 11; wrapMode: Text.WordWrap }
}
