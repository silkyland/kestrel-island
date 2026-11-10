import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The timer page in the island's amber language: the modes and the one
// coloured action sit in the header, the ruler and its clock are the page,
// and a Pomodoro adds one quiet line of break readouts below them. Running
// sessions replace the setup with the clock alone.
ColumnLayout {
    id: view
    spacing: 0
    readonly property var session: island.focus.snapshot
    readonly property bool completed: session.state === "completed"
    readonly property bool idle: session.state === "idle"
    property bool pomodoro: false
    property int minutes: 25
    function stamp(value) { return Math.floor(value/60).toString().padStart(2,"0")+":"+(value%60).toString().padStart(2,"0") }

    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 28
        spacing: 10
        Row {
            id: modeOptions
            objectName: "focusModeOptions"
            spacing: 14
            visible: view.idle
            ModeTab {
                objectName: "countdownMode"
                text: "Countdown"
                selected: !view.pomodoro
                onClicked: view.pomodoro = false
            }
            ModeTab {
                objectName: "pomodoroMode"
                text: "Pomodoro"
                selected: view.pomodoro
                onClicked: view.pomodoro = true
            }
        }
        Item { Layout.fillWidth: true }
        TimerPill {
            objectName: "startFocus"
            text: "Start"
            visible: view.idle
            onClicked: {
                if (view.pomodoro) {
                    island.focus.pomodoro(view.minutes, shortBreak.value, longBreak.value)
                    island.activity.select("focus")
                } else {
                    island.startTimer(view.minutes)
                }
            }
        }
        IslandButton {
            visible: !view.idle
            text: view.completed ? "Dismiss" : "Cancel"
            symbol: "process-stop"
            glyphSize: 16
            implicitWidth: 28
            implicitHeight: 28
            onClicked: island.cancelTimer()
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ColumnLayout {
            objectName: "focusDurationControls"
            visible: view.idle
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 12
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                TimerRuler {
                    id: ruler
                    Layout.fillWidth: true
                    Layout.preferredHeight: 82
                    value: view.minutes
                    onPicked: function(minute) { view.minutes = minute }
                }
                Text {
                    id: setupClock
                    textFormat: Text.PlainText
                    text: view.stamp(view.minutes * 60)
                    color: Theme.timer
                    font.pixelSize: 34
                    font.weight: Font.Light
                    font.features: {"tnum": 1}
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    Layout.preferredWidth: Math.max(86, implicitWidth)
                    Layout.preferredHeight: 82
                }
            }
            RowLayout {
                objectName: "focusPomodoroControls"
                visible: view.pomodoro
                Layout.alignment: Qt.AlignHCenter
                spacing: 8
                Text { textFormat: Text.PlainText; text: "Short"; color: Theme.secondary; font.pixelSize: 11 }
                IslandStepper { id: shortBreak; from: 1; to: 180; value: 5; implicitWidth: 80; implicitHeight: 26; Accessible.name: "Short break in minutes" }
                Text { textFormat: Text.PlainText; text: "Long"; color: Theme.secondary; font.pixelSize: 11 }
                IslandStepper { id: longBreak; from: 1; to: 180; value: 15; implicitWidth: 80; implicitHeight: 26; Accessible.name: "Long break in minutes, every four rounds" }
            }
        }

        ColumnLayout {
            visible: !view.idle
            anchors.centerIn: parent
            spacing: 6
            Text {
                textFormat: Text.PlainText
                Layout.alignment: Qt.AlignHCenter
                visible: view.completed
                text: "Session complete"
                color: Theme.timer
                font.pixelSize: 25
                font.weight: Font.Light
            }
            RollingText {
                Layout.alignment: Qt.AlignHCenter
                visible: !view.completed
                text: view.stamp(view.session.remaining)
                color: Theme.timer
                pixelSize: 46
                animate: island.animations
            }
            Text {
                textFormat: Text.PlainText
                Layout.alignment: Qt.AlignHCenter
                text: view.completed ? (view.session.pomodoro ? "Next: " + view.session.nextPhase : "Well done.") : (view.session.pomodoro ? view.session.phase + " · round " + view.session.round : view.session.state === "paused" ? "Paused" : "Countdown")
                color: Theme.secondary
                font.pixelSize: 12
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 8
                TimerPill {
                    visible: !view.completed
                    text: view.session.state === "paused" ? "Resume" : "Pause"
                    onClicked: island.pauseTimer()
                }
                TimerPill {
                    visible: view.completed && view.session.pomodoro
                    text: "Start next phase"
                    onClicked: island.focus.next()
                }
            }
        }
    }
}
