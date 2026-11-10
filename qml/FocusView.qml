import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    id: view
    spacing: 8
    readonly property var session: island.focus.snapshot
    readonly property bool completed: session.state === "completed"
    readonly property bool idle: session.state === "idle"
    property bool pomodoro: false
    function stamp(value) { return Math.floor(value/60).toString().padStart(2,"0")+":"+(value%60).toString().padStart(2,"0") }
    RowLayout {
        Layout.fillWidth: true
        Text { textFormat: Text.PlainText; text: "Focus"; color: Theme.ink; font.pixelSize: 20; font.weight: Font.DemiBold }
        Item { Layout.fillWidth: true }
        Text { textFormat: Text.PlainText; text: view.idle ? "Ready when you are" : view.completed ? "Finished" : view.session.state === "paused" ? "Paused" : "In progress"; color: Theme.secondary; font.pixelSize: 11 }
    }
    Item {
        visible: view.idle
        Layout.fillWidth: true
        implicitHeight: modeOptions.implicitHeight
        RowLayout {
            id: modeOptions
            objectName: "focusModeOptions"
            anchors.centerIn: parent
            IslandAction { text: "Countdown"; symbol: "timer"; checked: !view.pomodoro; onClicked: view.pomodoro=false }
            IslandAction { objectName: "pomodoroMode"; text: "Pomodoro"; symbol: "cycles"; checked: view.pomodoro; onClicked: view.pomodoro=true }
        }
    }
    Text { visible: view.completed; textFormat: Text.PlainText; Layout.alignment: Qt.AlignHCenter; text: "Session complete"; color: Theme.timer; font.pixelSize: 25; font.weight: Font.Light }
    RollingText { visible: !view.completed; Layout.alignment: Qt.AlignHCenter; text: view.stamp(view.idle ? minutes.value * 60 : view.session.remaining); color: Theme.timer; pixelSize: 44; animate: island.animations && !view.idle }
    Text { textFormat: Text.PlainText; Layout.alignment: Qt.AlignHCenter; text: view.idle ? (view.pomodoro ? "Work, then rest. Each phase starts with you." : "One task. A little uninterrupted time.") : (view.session.pomodoro ? view.session.phase + " · round " + view.session.round : "Countdown") ; color: Theme.secondary; font.pixelSize: 12 }
    RowLayout {
        objectName: "focusDurationControls"
        visible: view.idle
        Layout.alignment: Qt.AlignHCenter
        Text { textFormat: Text.PlainText; text: view.pomodoro ? "Work" : "Duration"; color: Theme.secondary; font.pixelSize: 12 }
        IslandStepper { id: minutes; objectName: "focusMinutes"; from: 1; to: 180; value: 25; implicitWidth: 104; implicitHeight: 32; Accessible.name: "Focus duration in minutes" }
        Text { textFormat: Text.PlainText; text: "min"; color: Theme.secondary; font.pixelSize: 11 }
        IslandAction { visible: !view.pomodoro; text: "25 min"; checked: minutes.value === 25; onClicked: minutes.value=25 }
        IslandAction { visible: !view.pomodoro; text: "50 min"; checked: minutes.value === 50; onClicked: minutes.value=50 }
    }
    RowLayout {
        objectName: "focusPomodoroControls"
        visible: view.idle && view.pomodoro
        Layout.alignment: Qt.AlignHCenter
        Text { textFormat: Text.PlainText; text: "Break"; color: Theme.secondary; font.pixelSize: 12 }
        IslandStepper { id: shortBreak; from: 1; to: 180; value: 5; implicitWidth: 90; implicitHeight: 32; Accessible.name: "Short break in minutes" }
        Text { textFormat: Text.PlainText; text: "Long"; color: Theme.secondary; font.pixelSize: 12 }
        IslandStepper { id: longBreak; from: 1; to: 180; value: 15; implicitWidth: 90; implicitHeight: 32; Accessible.name: "Long break in minutes, every four rounds" }
        Text { textFormat: Text.PlainText; text: "min"; color: Theme.secondary; font.pixelSize: 11 }
    }
    Item { Layout.fillHeight: true }
    RowLayout {
        Layout.fillWidth: true
        IslandAction { objectName: "startFocus"; visible: view.idle; Layout.fillWidth: true; primary: true; text: view.pomodoro ? "Start Pomodoro" : "Start countdown"; onClicked: { if(view.pomodoro) { island.focus.pomodoro(minutes.value,shortBreak.value,longBreak.value); island.activity.select("focus") } else island.startTimer(minutes.value) } }
        IslandAction { visible: !view.idle && !view.completed; Layout.fillWidth: true; primary: true; text: view.session.state === "paused" ? "Resume" : "Pause"; onClicked: island.pauseTimer() }
        IslandAction { visible: view.completed && view.session.pomodoro; Layout.fillWidth: true; primary: true; text: "Start next phase"; onClicked: island.focus.next() }
        IslandAction { visible: !view.idle; text: view.completed ? "Dismiss" : "Cancel"; onClicked: island.cancelTimer() }
    }
}
