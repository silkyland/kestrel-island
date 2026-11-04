import QtQuick

// 16 px surface-to-transparent gradients at the top and bottom edges of a
// Flickable, visible only while more content lies beyond that edge. Exact
// (no shaders, no mask) because the island surface is solid Theme.surface.
Item {
    id: fade
    required property Flickable target
    property int size: 16
    readonly property bool topMore: target.contentY > 1
    readonly property bool bottomMore: target.contentY + target.height < target.contentHeight - 1
    Rectangle {
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: fade.size
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.surface }
            GradientStop { position: 1; color: Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0) }
        }
        opacity: fade.topMore ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.dur(180) } }
    }
    Rectangle {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: fade.size
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0) }
            GradientStop { position: 1; color: Theme.surface }
        }
        opacity: fade.bottomMore ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.dur(180) } }
    }
}
