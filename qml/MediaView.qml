import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: view
    readonly property color ink: "#f5f5f7"
    readonly property color secondary: "#a7a7af"
    readonly property color accent: "#b8a1ff"
    readonly property bool available: island.media.available === true
    readonly property bool playing: island.media.playing === true
    readonly property color tint: island.media.tint ? island.media.tint : "#b8a1ff"
    function stamp(value) { value = Math.max(0, Number.isFinite(value) ? value : 0); return Math.floor(value/60).toString()+":"+Math.floor(value%60).toString().padStart(2,"0") }

    spacing: 0
    // Head: cover, title and artist, then the live bars — the player's first row.
    RowLayout {
        Layout.fillWidth: true; spacing: 14
        Rectangle {
            Layout.preferredWidth: 68; Layout.preferredHeight: 68
            Layout.alignment: Qt.AlignTop
            radius: 13; color: "#202023"; clip: true
            scale: view.playing ? 1 : 0.94
            Behavior on scale { NumberAnimation { duration: island.animations ? 300 : 0; easing.type: Easing.OutCubic } }
            Image {
                id: albumArt
                anchors.fill: parent
                // Read local artwork only; the app makes no remote requests.
                source: island.media.art && island.media.art.toString().startsWith("file://") ? island.media.art : ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }
            Image { anchors.centerIn: parent; width: 30; height: 30; source: "qrc:/assets/icons/music.svg"; opacity: 0.45; visible: albumArt.status !== Image.Ready }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 2
            Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: island.media.title || (view.available ? "Unknown track" : "Nothing playing"); color: view.ink; font.pixelSize: 14; font.weight: Font.DemiBold; elide: Text.ElideRight; maximumLineCount: 1; ToolTip.visible: titleHover.hovered; ToolTip.text: text; HoverHandler { id: titleHover } }
            Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: view.available ? (island.media.artist || island.media.album || "Now playing") : "Play something in Spotify, a browser or VLC."; color: view.secondary; font.pixelSize: 12; elide: Text.ElideRight; maximumLineCount: 1 }
        }
        Row {
            visible: view.available
            spacing: 3
            Layout.alignment: Qt.AlignVCenter
            Repeater {
                model: 4
                Rectangle {
                    required property int index
                    width: 2.5; radius: 1.2; color: view.tint
                    height: view.playing ? [7, 13, 10, 15][index] : 4
                    anchors.verticalCenter: parent.verticalCenter
                    SequentialAnimation on height {
                        running: view.playing && root.visible && island.animations
                        loops: Animation.Infinite
                        NumberAnimation { to: 4; duration: 360 + index * 75; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 8 + index * 2.5; duration: 420 + index * 65; easing.type: Easing.InOutSine }
                    }
                }
            }
        }
    }
    // Timeline across the island: elapsed left, remaining right.
    RowLayout {
        Layout.fillWidth: true; Layout.topMargin: 14; spacing: 8
        visible: island.media.durationUs > 0
        Text { textFormat: Text.PlainText; text: view.stamp(timeline.value); color: view.secondary; font.pixelSize: 10; Layout.preferredWidth: 32 }
        Slider {
            id: timeline
            objectName: "mediaTimeline"
            // Custom visual items have no implicit size in the KDE style.
            implicitHeight: 24
            Layout.fillWidth: true
            from: 0; to: Math.max(1, island.media.durationUs / 1000000)
            enabled: island.media.canSeek === true && !!island.media.trackId
            property string dragService: ""
            property string dragTrack: ""
            property double dragGeneration: 0
            Accessible.name: "Track position"
            onPressedChanged: {
                if (pressed) { dragService=island.media.service; dragTrack=island.media.trackId; dragGeneration=island.seekGeneration; seekHold.stop() }
                else { island.seek(value,dragService,dragTrack,dragGeneration); seekHold.restart() }
            }
            onMoved: if (!pressed) island.seek(value,island.media.service,island.media.trackId,island.seekGeneration)
            Binding { target: timeline; property: "value"; value: island.playbackPosition; when: !timeline.pressed && !seekHold.running }
            Timer { id: seekHold; interval: 1500 }
            Connections { target: island; function onMediaChanged() { if (timeline.dragTrack !== island.media.trackId || timeline.dragService !== island.media.service) seekHold.stop() } }
            background: Rectangle { x: timeline.leftPadding; y: timeline.topPadding + timeline.availableHeight/2-2; width: timeline.availableWidth; height: 4; radius: 2; color: "#303035"; Rectangle { width: timeline.visualPosition*parent.width; height: 4; radius: 2; color: view.tint } }
            handle: Rectangle { x: timeline.leftPadding + timeline.visualPosition*(timeline.availableWidth-width); y: timeline.topPadding+timeline.availableHeight/2-5; width: 10; height: 10; radius: 5; color: timeline.enabled ? "#f5f5f7" : "#66666c"; border.width: timeline.activeFocus ? 2 : 0; border.color: view.accent }
        }
        Text { textFormat: Text.PlainText; text: "-" + view.stamp(island.media.durationUs / 1000000 - timeline.value); color: view.secondary; font.pixelSize: 10; Layout.preferredWidth: 32; horizontalAlignment: Text.AlignRight }
    }
    Text { textFormat: Text.PlainText; Layout.fillWidth: true; Layout.topMargin: 14; visible: view.available && !(island.media.durationUs > 0); text: "This player does not provide a timeline."; color: view.secondary; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter }
    // Five equal places: the transport keeps the middle three.
    RowLayout {
        Layout.fillWidth: true; Layout.topMargin: 10
        Item { Layout.preferredWidth: view.width / 5; Layout.preferredHeight: 44
            IslandButton { anchors.centerIn: parent; text: "Switch activity"; symbol: "activities"; implicitWidth: 36; implicitHeight: 36; dim: true; visible: island.activity.activities.length > 1; onClicked: island.activity.cycle() }
        }
        RowLayout {
            Layout.preferredWidth: view.width / 5 * 3; Layout.preferredHeight: 44
            spacing: 0
            Item { Layout.fillWidth: true }
            IslandButton { text: "Previous track"; symbol: "media-skip-backward"; implicitWidth: 36; implicitHeight: 36; dim: !view.available; enabled: island.media.canPrevious === true; onClicked: island.mediaAction("Previous") }
            IslandButton { text: view.playing ? "Pause" : "Play"; symbol: view.playing ? "media-playback-pause" : "media-playback-start"; prominent: true; implicitWidth: 44; implicitHeight: 44; enabled: view.playing ? island.media.canPause === true : island.media.canPlay === true; onClicked: island.mediaAction("PlayPause") }
            IslandButton { text: "Next track"; symbol: "media-skip-forward"; implicitWidth: 36; implicitHeight: 36; dim: !view.available; enabled: island.media.canNext === true; onClicked: island.mediaAction("Next") }
            Item { Layout.fillWidth: true }
        }
        Item { Layout.preferredWidth: view.width / 5; Layout.preferredHeight: 44 }
    }
    Item { Layout.fillHeight: true }
}
