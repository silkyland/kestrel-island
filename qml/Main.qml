import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

Window {
    id: root
    width: 588
    height: 594
    visible: false
    color: "transparent"
    title: "Kestrel Island"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    property bool expanded: startExpanded
    property bool pinned: false
    onPinnedChanged: if (!pinned && expanded && !islandHover.hovered) leaveDelay.restart()
    property bool hoverBlocked: false
    property int page: 0
    onPageChanged: { island.setCurrentPage(page); if (expanded && island.animations) pageFade.restart() }
    onExpandedChanged: { island.setExpanded(expanded); if (expanded) peek = false }
    property bool peek: false
    property bool noticeBump: false
    property bool noticeCard: false
    property string lastNotice
    readonly property string noticeSymbol: {
        const key = String(island.activity.notice.key || "")
        if (key === "language") return "language"
        if (key.indexOf("notification:") === 0) return "bell"
        if (key.indexOf("task:") === 0) return "terminal"
        if (key === "focus-complete") return "timer"
        return "activities"
    }
    Component.onCompleted: { page = island.preferredPage; island.setCurrentPage(page); island.setExpanded(expanded) }
    readonly property color ink: Theme.ink
    readonly property color secondary: Theme.secondary
    readonly property color accent: Theme.accent
    readonly property bool playing: island.media.playing === true
    readonly property bool available: island.media.available === true
    // A transient failure text adds a row below the page; the surface grows
    // by that row so the page keeps its exact bands instead of being clipped.
    readonly property int messageHeight: island.message.length > 0 ? Math.ceil(messageLabel.implicitHeight) + 8 : 0

    function collapse() { peekDelay.stop(); leaveDelay.stop(); peek = false; noticeBump = false; noticeCard = false; hoverBlocked = islandHover.hovered; expanded = false; pinned = false }
    // An outside click steals window activation; that is the dismiss path
    // unless the island is explicitly pinned.
    function dismissOutside() { if (expanded && !pinned) collapse() }
    onActiveChanged: if (!active) dismissOutside()
    function navigate(target) { page = target; island.setPreferredPage(target) }
    function toggle() { if (expanded) { collapse(); return } page = island.preferredPage; expanded = true }
    function showSettings() { page = 3; expanded = true }
    function timeString(seconds) { return Math.floor(seconds / 60).toString().padStart(2, "0") + ":" + (seconds % 60).toString().padStart(2, "0") }

    Connections {
        target: island
        function onToggleRequested() { root.toggle() }
        function onExpansionRequested(expanded) { if (expanded) { root.expanded = true } else root.collapse() }
        function onSettingsRequested() { root.showSettings() }
        function onTimerFinished() { /* Completion is a compact notice, never a focus grab. */ }
    }
    // Escape closes the inner layer first: tool child pages step back to
    // Tools, everything else collapses the island.
    Shortcut { sequence: "Escape"; onActivated: { if (root.page >= 4 && root.page <= 6) root.navigate(7); else root.collapse() } }
    Connections {
        target: island.activity
        // A fresh notice nods the compact surface once (same nod geometry as
        // the hover peek); the title itself cross-fades until expiry.
        function onChanged() {
            const stamp = String(island.activity.notice.key || "") + ":" + String(island.activity.notice.seq || "")
            if (stamp === root.lastNotice) return
            root.lastNotice = stamp
            if (island.activity.notice.key && !root.expanded && island.animations) {
                if (island.activity.notice.card) {
                    root.noticeCard = true
                    noticeCardDelay.restart()
                } else {
                    root.noticeBump = true
                    noticeBumpDelay.restart()
                }
            }
        }
    }
    Timer { id: peekDelay; interval: 120; onTriggered: if (!root.expanded && !root.hoverBlocked && island.hoverExpand && island.animations) root.peek = true }
    Timer { id: noticeBumpDelay; interval: 800; onTriggered: root.noticeBump = false }
    Timer { id: noticeCardDelay; interval: 1600; onTriggered: root.noticeCard = false }
    Timer { id: leaveDelay; interval: 180; onTriggered: if (!root.pinned) root.expanded = false }

    Item {
        id: surface
        anchors.horizontalCenter: parent.horizontalCenter
        objectName: "islandSurface"
        width: root.expanded ? (root.page === 2 ? (systemPage.details ? 560 : 520) : root.page === 0 ? 424 : 456) : (root.peek || root.noticeBump ? 316 : 288)
        height: root.expanded ? ((root.page === 2 ? (systemPage.details ? 568 : 432) : root.page === 3 ? 400 : root.page === 1 ? (focusPage.pomodoro ? 240 : 216) : root.page === 5 || root.page === 6 ? 384 : root.page === 4 ? 336 : root.page === 0 ? (island.media.durationUs > 0 ? 254 : 216) : 288) + root.messageHeight) : (root.peek || root.noticeBump ? 54 : 40)
        clip: true
        Behavior on width { NumberAnimation { duration: Theme.dur(280); easing.type: Easing.OutCubic } }
        Behavior on height { NumberAnimation { duration: Theme.dur(280); easing.type: Easing.OutCubic } }
        onWidthChanged: region.restart()
        onHeightChanged: region.restart()
        Timer { id: region; interval: 0; onTriggered: island.updateInputRegion(surface.x, surface.width, surface.height) }
        Component.onCompleted: region.restart()

        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                fillColor: Theme.surface
                strokeWidth: -1
                startX: 0; startY: 0
                PathQuad { x: 14; y: 14; controlX: 14; controlY: 0 }
                PathLine { x: 14; y: surface.height - Math.min(26, surface.height / 2) }
                PathQuad { x: 14 + Math.min(26, surface.height / 2); y: surface.height; controlX: 14; controlY: surface.height }
                PathLine { x: surface.width - 14 - Math.min(26, surface.height / 2); y: surface.height }
                PathQuad { x: surface.width - 14; y: surface.height - Math.min(26, surface.height / 2); controlX: surface.width - 14; controlY: surface.height }
                PathLine { x: surface.width - 14; y: 14 }
                PathQuad { x: surface.width; y: 0; controlX: surface.width - 14; controlY: 0 }
                PathLine { x: 0; y: 0 }
            }
        }

        HoverHandler {
            id: islandHover
            objectName: "islandHover"
            onHoveredChanged: {
                if (hovered) { leaveDelay.stop(); if (!root.hoverBlocked) peekDelay.restart() }
                else { if (root.visible) root.hoverBlocked = false; peekDelay.stop(); root.peek = false }
            }
        }
        TapHandler { acceptedButtons: Qt.RightButton; onTapped: island.openMenu() }

        RowLayout {
            id: compact
            anchors { top: parent.top; left: parent.left; right: parent.right; leftMargin: 26; rightMargin: 26 }
            height: root.expanded ? 40 : surface.height
            spacing: 6
            opacity: root.expanded || root.noticeCard ? 0 : 1
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: Theme.dur(100) } }
            Item {
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Rectangle {
                    anchors.fill: parent
                    readonly property string artwork: root.available && island.activity.selected.key === "music" && !island.activity.notice.title ? String(island.media.art || "") : ""
                    radius: width * 0.19
                    color: Theme.raised
                    clip: true
                    Image {
                        id: compactArtwork
                        objectName: "compactArtwork"
                        anchors.fill: parent
                        source: parent.artwork.startsWith("file://") ? parent.artwork : ""
                        sourceSize.width: 48; sourceSize.height: 48
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        visible: status === Image.Ready
                    }
                    Rectangle { anchors.fill: parent; radius: parent.radius; color: "transparent"; border.width: 0.5; border.color: Theme.hairline }
                }
                Image {
                    anchors.centerIn: parent
                    width: 20; height: 20
                    source: "qrc:/assets/island.svg"
                    sourceSize.width: 22; sourceSize.height: 22
                    opacity: 0.45
                    visible: compactArtwork.status !== Image.Ready
                }
            }
            Item {
                // Compact title cross-fades when the activity or notice
                // changes; numerals use tabular figures (decision 9).
                Layout.fillWidth: true
                implicitHeight: compactTitle.implicitHeight
                property string value: island.activity.notice.title || island.activity.selected.title || "Kestrel Island"
                property string shown: value
                onValueChanged: {
                    if (shown === value) return
                    previousTitle.text = shown
                    shown = value
                    titleSwap.restart()
                }
                component CompactTitle: Text {
                    textFormat: Text.PlainText
                    width: parent.width
                    color: root.ink
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    font.features: {"tnum": 1}
                    elide: Text.ElideRight
                }
                CompactTitle { id: previousTitle; opacity: 0 }
                CompactTitle { id: compactTitle; text: parent.shown; opacity: 1 - previousTitle.opacity }
                SequentialAnimation {
                    id: titleSwap
                    PropertyAction { target: previousTitle; property: "opacity"; value: 1 }
                    NumberAnimation { target: previousTitle; property: "opacity"; to: 0; duration: Theme.dur(140) }
                }
            }
            Row {
                Layout.preferredWidth: 21
                spacing: 3
                Repeater {
                    model: 4
                    Rectangle {
                        required property int index
                        width: 3; radius: 1.5; color: island.remaining > 0 ? Theme.timer : Theme.tint
                        height: root.playing ? [7, 15, 11, 18][index] : 5
                        anchors.verticalCenter: parent.verticalCenter
                        SequentialAnimation on height {
                            running: root.playing && !root.expanded && root.visible && island.animations
                            loops: Animation.Infinite
                            NumberAnimation { to: 5; duration: Theme.dur(360 + index * 75); easing.type: Easing.InOutSine }
                            NumberAnimation { to: 10 + index * 3; duration: Theme.dur(420 + index * 65); easing.type: Easing.InOutSine }
                        }
                    }
                }
            }
            IslandButton { text: "Switch activity"; symbol: "activities"; implicitWidth: 22; implicitHeight: 24; padding: 4; visible: island.activity.activities.length > 1; onClicked: island.activity.cycle() }
            IslandButton {
                objectName: "compactSettingsButton"
                text: "Settings"
                symbol: "configure"
                implicitWidth: 24; implicitHeight: 24; padding: 4
                onClicked: root.showSettings()
            }
        }

        // Transient event card: same compact frame, but only the event icon
        // and title show while every other control stays hidden.
        Row {
            anchors.centerIn: parent
            spacing: 8
            opacity: !root.expanded && root.noticeCard ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: Theme.dur(140) } }
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 18; height: 18
                source: "qrc:/assets/icons/" + root.noticeSymbol + ".svg"
                sourceSize.width: 20; sourceSize.height: 20
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                textFormat: Text.PlainText
                text: island.activity.notice.title || ""
                color: root.ink
                font.pixelSize: 14
                font.weight: Font.Medium
                font.features: {"tnum": 1}
                elide: Text.ElideRight
                width: Math.min(implicitWidth, 220)
            }
        }

        MouseArea {
            objectName: "compactMoveHandle"
            x: 24; y: 0; width: surface.width - (island.activity.activities.length > 1 ? 110 : 80); height: surface.height
            visible: !root.expanded
            acceptedButtons: Qt.LeftButton
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.PointingHandCursor
            property point startPoint
            property bool dragged: false
            onPressed: function(mouse) { startPoint = Qt.point(mouse.x, mouse.y); dragged = false }
            onPositionChanged: function(mouse) {
                if (pressed && !dragged && Math.hypot(mouse.x - startPoint.x, mouse.y - startPoint.y) > 10) {
                    dragged = true
                    root.collapse()
                    island.beginMove()
                }
            }
            onClicked: if (!dragged) { root.page = island.preferredPage; root.expanded = true }
        }

        ColumnLayout {
            id: content
            anchors { fill: parent; leftMargin: 34; rightMargin: 34; topMargin: 10; bottomMargin: 14 }
            spacing: 8
            opacity: root.expanded && surface.height > 200 ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: Theme.dur(140) } }

            Item {
                objectName: "islandNavigation"
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                // Page tabs take the leading edge, the window actions the
                // trailing edge: the same inset as the player below.
                RowLayout {
                    id: primaryTabs
                    objectName: "primaryTabs"
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Repeater {
                        model: [
                            {name: "Music", symbol: "music"},
                            {name: "Focus", symbol: "timer"},
                            {name: "System", symbol: "cpu"},
                            {name: "Tools", symbol: "tools"}
                        ]
                        IslandButton {
                            required property int index
                            required property var modelData
                            text: modelData.name + " tab"
                            label: modelData.name
                            symbol: modelData.symbol
                            glyphSize: 18
                            idleWidth: 34
                            implicitHeight: 32
                            dim: true
                            selected: index === 3 ? root.page >= 4 : root.page === index
                            onClicked: root.navigate(index === 3 ? 7 : index)
                        }
                    }
                }
                RowLayout {
                    id: navigationActions
                    objectName: "navigationActions"
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    IslandButton { text: "Switch activity"; symbol: "activities"; glyphSize: 18; dim: true; implicitWidth: 30; implicitHeight: 30; visible: island.activity.activities.length > 1; onClicked: { island.activity.cycle(); root.page = island.activity.selected.page || 0; } }
                    IslandButton {
                        objectName: "pinButton"
                        text: root.pinned ? "Unpin island" : "Pin island"
                        symbol: root.pinned ? "pin-filled" : "pin"
                        glyphSize: 18
                        dim: true
                        implicitWidth: 30; implicitHeight: 30
                        checkable: true
                        checked: root.pinned
                        onClicked: root.pinned = checked
                    }
                    IslandButton { text: "Settings"; symbol: "configure"; glyphSize: 18; dim: true; implicitWidth: 30; implicitHeight: 30; onClicked: root.showSettings() }
                    IslandButton { text: "Collapse"; symbol: "arrow-up"; glyphSize: 18; dim: true; implicitWidth: 30; implicitHeight: 30; onClicked: root.collapse() }
                }
            }

            StackLayout {
                id: pageStack
                currentIndex: root.page
                NumberAnimation on opacity { id: pageFade; running: false; from: 0; to: 1; duration: Theme.dur(160); easing.type: Easing.OutCubic }
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 0

                MediaView {}

                FocusView { id: focusPage }

                SystemView { id: systemPage }
                Item {
                    ScrollView {
                        id: settingsScroll
                        anchors.fill: parent
                        clip: true
                        contentWidth: availableWidth
                        ColumnLayout {
                        width: settingsScroll.availableWidth
                        objectName: "settingsPage"
                        spacing: 4
                        Text { textFormat: Text.PlainText; text: "Island settings"; color: root.ink; font.pixelSize: 20; font.weight: Font.DemiBold }
                        RowLayout {
                            objectName: "settingsShortcuts"
                            IslandAction { Layout.fillWidth: true; Layout.preferredWidth: 1; text: "Keep awake"; symbol: "coffee"; onClicked: { root.page=4; } }
                            IslandAction { Layout.fillWidth: true; Layout.preferredWidth: 1; text: "Tasks"; symbol: "terminal"; onClicked: { root.page=5; } }
                            IslandAction { Layout.fillWidth: true; Layout.preferredWidth: 1; text: "Inbox"; symbol: "bell"; onClicked: { root.page=6; } }
                        }
                        Text { textFormat: Text.PlainText; text: "POSITION"; color: root.secondary; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 1; Layout.topMargin: 6 }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { textFormat: Text.PlainText; text: "Display"; color: root.secondary; font.pixelSize: 12; Layout.preferredWidth: 70 }
                            IslandSelect {
                                id: displaySelector
                                objectName: "displaySelector"
                                Layout.fillWidth: true
                                implicitHeight: 32
                                model: island.displays
                                currentIndex: island.displays.indexOf(island.display)
                                displayText: currentText || "Default display"
                                onActivated: island.chooseDisplay(currentText)
                                Accessible.name: "Island display"
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { textFormat: Text.PlainText; text: "Top offset"; color: root.secondary; font.pixelSize: 12; Layout.preferredWidth: 70 }
                            IslandStepper {
                                from: 0; to: 120; stepSize: 8; value: island.topGap
                                implicitHeight: 32
                                onValueModified: island.setTopGap(value)
                                Accessible.name: "Top offset in pixels"
                            }
                            Text { textFormat: Text.PlainText; text: "px"; color: root.secondary; font.pixelSize: 12 }
                        }
                        Text { textFormat: Text.PlainText; text: "BEHAVIOR"; color: root.secondary; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 1; Layout.topMargin: 8 }
                        IslandCheck {
                            text: "Peek on hover"
                            checked: island.hoverExpand
                            implicitHeight: 30
                            onToggled: island.setHoverExpand(checked)
                        }
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: "Hover gives a peek. Click to open; it stays until you close it.\nDrag the compact title to another display."; color: root.secondary; font.pixelSize: 11; wrapMode: Text.WordWrap }
                        RowLayout {
                            Layout.fillWidth: true
                            IslandCheck { Layout.fillWidth: true; text: "Audio / microphone feedback"; checked: island.audioFeedback; onToggled: island.setAudioFeedback(checked) }
                            IslandButton {
                                visible: island.audioFeedback && island.micAvailable
                                text: island.micMuted ? "Unmute default microphone" : "Mute default microphone"
                                symbol: island.micMuted ? "microphone-muted" : "microphone"
                                implicitWidth: 32; implicitHeight: 32
                                onClicked: island.toggleMicMute()
                            }
                        }
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: "New activity sources are optional. Music keeps its automatic source selection."; color: root.secondary; font.pixelSize: 11; wrapMode: Text.WordWrap }
                        }
                    }
                    EdgeFade { anchors.fill: parent; target: settingsScroll.contentItem }
                }
                KeepAwakeView { objectName: "awakePage"; onBackRequested: root.navigate(7) }
                TasksView { objectName: "tasksPage"; onBackRequested: root.navigate(7) }
                NotificationsView { objectName: "notificationsPage"; onBackRequested: root.navigate(7) }
                ToolsView { objectName: "toolsPage"; onOpenPage: function(page) { root.navigate(page) } }

            }

            // The failure line takes a real row; the surface grew by
            // messageHeight above, so the page bands above keep their size.
            Text {
                id: messageLabel
                objectName: "messageLabel"
                Layout.fillWidth: true
                visible: island.message.length > 0
                textFormat: Text.PlainText
                text: island.message
                color: Theme.timer
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }
    }
}
