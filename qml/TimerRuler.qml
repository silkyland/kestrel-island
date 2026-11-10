import QtQuick
import QtQuick.Shapes

// A duration ruler in the island's timer language: the selected minute sits
// under a fixed amber pointer, the ticks slide beneath it, and the numbers
// on every fifth minute carry the scale. Drag horizontally to scrub, click a
// tick to jump, or use the arrow keys.
Item {
    id: ruler
    property int value: 25
    property int minimum: 1
    property int maximum: 180
    property color accent: Theme.timer
    property int step: 12
    signal picked(int minute)

    readonly property int span: Math.max(8, Math.floor(width / step))
    readonly property int first: value - Math.floor(span * 0.42)
    readonly property real selectionX: (value - first) * step
    implicitHeight: 82
    implicitWidth: 280
    activeFocusOnTab: true
    Accessible.role: Accessible.Slider
    Accessible.name: "Focus duration in minutes"
    Accessible.description: value + " minutes"

    function minuteAt(x) { return Math.max(minimum, Math.min(maximum, first + Math.round(x / step))) }
    function edgeFade(x) {
        const edge = Math.max(1, width * 0.10)
        return Math.max(0, Math.min(1, Math.min(x, width - x) / edge))
    }

    Repeater {
        model: ruler.maximum - ruler.minimum + 1
        delegate: Item {
            required property int index
            readonly property int minute: ruler.minimum + index
            readonly property real at: (minute - ruler.first) * ruler.step
            visible: at >= -14 && at <= ruler.width + 14
            width: 1; height: 1
            Rectangle {
                x: parent.at - 2; y: 24; width: 4; height: 38; radius: 2
                color: ruler.accent
                opacity: (parent.minute === ruler.value ? 1 : parent.minute < ruler.value ? 0.9 : 0.3) * ruler.edgeFade(parent.at)
            }
            Text {
                visible: parent.minute % 5 === 0 || parent.minute === 1
                x: parent.at - width / 2; y: 0
                width: 32; horizontalAlignment: Text.AlignHCenter
                text: parent.minute
                color: ruler.accent
                opacity: (parent.minute <= ruler.value ? 0.95 : 0.45) * ruler.edgeFade(parent.at)
                font.pixelSize: 11
                font.features: {"tnum": 1}
            }
        }
    }

    Shape {
        x: ruler.selectionX - 6
        y: 66
        width: 12; height: 10
        visible: ruler.selectionX > -8 && ruler.selectionX < ruler.width + 8
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            fillColor: ruler.accent
            strokeWidth: -1
            startX: 0; startY: 0
            PathLine { x: 12; y: 0 }
            PathLine { x: 6; y: 9 }
            PathLine { x: 0; y: 0 }
        }
    }

    Rectangle {
        x: 0; y: 0; width: 18; height: ruler.height; z: 2
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: Theme.surface }
            GradientStop { position: 1; color: "transparent" }
        }
    }
    Rectangle {
        x: ruler.width - 18; y: 0; width: 18; height: ruler.height; z: 2
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: "transparent" }
            GradientStop { position: 1; color: Theme.surface }
        }
    }
    Rectangle {
        anchors.fill: parent; radius: 8; color: "transparent"; z: 3
        border.width: 1; border.color: ruler.accent
        visible: ruler.activeFocus
    }

    MouseArea {
        id: scrub
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        property real fromX: 0
        property int fromMinute: 0
        property bool moved: false
        onPressed: function(mouse) {
            fromX = mouse.x
            fromMinute = ruler.value
            moved = false
            ruler.forceActiveFocus()
        }
        onPositionChanged: function(mouse) {
            if (!pressed)
                return
            const delta = Math.round((mouse.x - fromX) / ruler.step)
            if (delta === 0)
                return
            moved = true
            ruler.picked(Math.max(ruler.minimum, Math.min(ruler.maximum, fromMinute + delta)))
        }
        onReleased: function(mouse) {
            if (!moved) {
                const minute = ruler.minuteAt(mouse.x)
                if (minute !== ruler.value)
                    ruler.picked(minute)
            }
        }
        onWheel: function(wheel) {
            ruler.picked(Math.max(ruler.minimum, Math.min(ruler.maximum, ruler.value + (wheel.angleDelta.y > 0 ? 1 : -1))))
        }
    }

    Keys.onLeftPressed: ruler.picked(Math.max(ruler.minimum, ruler.value - 1))
    Keys.onDownPressed: ruler.picked(Math.max(ruler.minimum, ruler.value - 1))
    Keys.onRightPressed: ruler.picked(Math.min(ruler.maximum, ruler.value + 1))
    Keys.onUpPressed: ruler.picked(Math.min(ruler.maximum, ruler.value + 1))
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Home) { ruler.picked(ruler.minimum); event.accepted = true }
        else if (event.key === Qt.Key_End) { ruler.picked(ruler.maximum); event.accepted = true }
    }
}
