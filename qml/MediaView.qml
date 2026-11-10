import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The player as a phone's island lays it out: a 56 px cover with the song
// beside it, the timeline across the island, one row of five equal places
// with the transport in the middle three, and the audio footer directly
// under it. Nothing here floats: the page is exactly as tall as these four
// bands, so the footer lands 12 px above the island's lower edge.
ColumnLayout {
    id: view
    readonly property color ink: Theme.ink
    readonly property color secondary: Theme.secondary
    readonly property bool available: island.media.available === true
    readonly property bool playing: island.media.playing === true
    readonly property bool hasTimeline: island.media.durationUs > 0
    readonly property string loop: String(island.media.loop || "None")
    function stamp(value) { value = Math.max(0, Number.isFinite(value) ? value : 0); return Math.floor(value/60).toString()+":"+Math.floor(value%60).toString().padStart(2,"0") }
    // The remaining side rounds up so elapsed + remaining always equals the
    // track's length, as a phone's player shows it.
    function remains(value) { value = Math.max(0, Number.isFinite(value) ? value : 0); const total = Math.ceil(value); return Math.floor(total/60).toString()+":"+(total%60).toString().padStart(2,"0") }

    spacing: 0

    // Head: cover, title and artist, then the live bars at the far end.
    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 56
        spacing: 14
        Rectangle {
            Layout.preferredWidth: 56; Layout.preferredHeight: 56
            Layout.alignment: Qt.AlignBottom
            radius: width * 0.19; color: Theme.raised; clip: true
            scale: view.playing ? 1 : 0.94
            Behavior on scale { NumberAnimation { duration: Theme.dur(300); easing.type: Easing.OutCubic } }
            Image {
                id: albumArt
                anchors.fill: parent
                // Read local artwork only; the app makes no remote requests.
                source: island.media.art && island.media.art.toString().startsWith("file://") ? island.media.art : ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }
            Rectangle { anchors.fill: parent; radius: parent.radius; color: "transparent"; border.width: 0.5; border.color: Theme.hairline }
            Image { anchors.centerIn: parent; width: 22; height: 22; source: "qrc:/assets/icons/music.svg"; opacity: 0.45; visible: albumArt.status !== Image.Ready }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.alignment: Qt.AlignBottom; spacing: 2
            Text {
                textFormat: Text.PlainText; Layout.fillWidth: true
                text: island.media.title || (view.available ? "Unknown track" : "Nothing playing")
                color: view.ink; font.pixelSize: 14; font.weight: Font.DemiBold
                elide: Text.ElideRight; maximumLineCount: 1
                ToolTip.visible: titleHover.hovered; ToolTip.text: text
                HoverHandler { id: titleHover }
            }
            Text {
                textFormat: Text.PlainText; Layout.fillWidth: true
                text: view.available ? (island.media.artist || island.media.album || "Now playing") : "Start a song in any player and it will appear here."
                color: view.secondary; font.pixelSize: 13
                elide: Text.ElideRight; maximumLineCount: 1
            }
        }
        Row {
            visible: view.available
            spacing: 3
            Layout.alignment: Qt.AlignVCenter
            Repeater {
                model: 4
                Rectangle {
                    required property int index
                    width: 2.5; radius: 1.2; color: Theme.tint
                    height: view.playing ? [7, 13, 10, 15][index] : 4
                    anchors.verticalCenter: parent.verticalCenter
                    SequentialAnimation on height {
                        running: view.playing && root.visible && island.animations
                        loops: Animation.Infinite
                        NumberAnimation { to: 4; duration: Theme.dur(360 + index * 75); easing.type: Easing.InOutSine }
                        NumberAnimation { to: 8 + index * 2.5; duration: Theme.dur(420 + index * 65); easing.type: Easing.InOutSine }
                    }
                }
            }
        }
    }

    // Timeline: only a player that reports a length gets this band, so the
    // idle player stays compact and the island grows when a track starts.
    // The band and the surface animate with the same 280 ms curve, which
    // keeps the four bands exactly as tall as the page at every frame.
    Item {
        id: timelineBand
        Layout.fillWidth: true
        Layout.preferredHeight: 38 * reveal
        clip: true
        property real reveal: view.hasTimeline ? 1 : 0
        Behavior on reveal { NumberAnimation { duration: Theme.dur(280); easing.type: Easing.OutCubic } }
        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 14
            height: 24
            opacity: timelineBand.reveal
            enabled: view.hasTimeline
            spacing: 8
            Text { textFormat: Text.PlainText; text: view.stamp(timeline.value); color: view.secondary; font.pixelSize: 10; font.features: {"tnum": 1}; Layout.preferredWidth: 30 }
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
                background: Rectangle { x: timeline.leftPadding; y: timeline.topPadding + timeline.availableHeight/2-2; width: timeline.availableWidth; height: 4; radius: 2; color: Theme.track; Rectangle { width: timeline.visualPosition*parent.width; height: 4; radius: 2; color: Theme.tint } }
                handle: Rectangle { x: timeline.leftPadding + timeline.visualPosition*(timeline.availableWidth-width); y: timeline.topPadding+timeline.availableHeight/2-5; width: 10; height: 10; radius: 5; color: timeline.enabled ? Theme.ink : Theme.faint; border.width: timeline.activeFocus ? 2 : 0; border.color: Theme.tint }
            }
            Text { textFormat: Text.PlainText; text: "\u2212" + view.remains(island.media.durationUs / 1000000 - timeline.value); color: view.secondary; font.pixelSize: 10; font.features: {"tnum": 1}; Layout.preferredWidth: 30; horizontalAlignment: Text.AlignRight }
        }
    }

    // Five equal places across the island: shuffle before the transport and
    // repeat after it, each place kept when its button is not offered so
    // play stays in the middle.
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: 54
        RowLayout {
            anchors.fill: parent
            anchors.topMargin: 10
            spacing: 0
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                IslandButton {
                    anchors.centerIn: parent
                    width: 36; height: 36; glyphSize: 16; circular: true
                    symbol: "shuffle"; selected: island.media.shuffle === true
                    text: selected ? "Turn shuffle off" : "Turn shuffle on"
                    visible: island.media.canShuffle === true
                    onClicked: island.setShuffle(!selected)
                }
            }
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                IslandButton {
                    anchors.centerIn: parent
                    width: 44; height: 44; glyphSize: 19; circular: true
                    text: "Previous track"; symbol: "media-skip-backward"
                    enabled: island.media.canPrevious === true
                    onClicked: island.mediaAction("Previous")
                }
            }
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                IslandButton {
                    anchors.centerIn: parent
                    width: 44; height: 44; glyphSize: 28; circular: true
                    text: view.playing ? "Pause" : "Play"
                    symbol: view.playing ? "media-playback-pause" : "media-playback-start"
                    enabled: view.playing ? island.media.canPause === true : island.media.canPlay === true
                    onClicked: island.mediaAction("PlayPause")
                }
            }
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                IslandButton {
                    anchors.centerIn: parent
                    width: 44; height: 44; glyphSize: 19; circular: true
                    text: "Next track"; symbol: "media-skip-forward"
                    enabled: island.media.canNext === true
                    onClicked: island.mediaAction("Next")
                }
            }
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                IslandButton {
                    anchors.centerIn: parent
                    width: 36; height: 36; glyphSize: 16; circular: true
                    symbol: view.loop === "Track" ? "repeat-one" : "repeat"
                    selected: view.loop !== "None"
                    text: view.loop === "None" ? "Repeat all" : view.loop === "Playlist" ? "Repeat one" : "Turn repeat off"
                    visible: island.media.canLoop === true
                    onClicked: island.cycleLoop()
                }
            }
        }
    }

    // Audio lives with the player, not in a separate toolbar at the bottom
    // edge: the speaker mutes, the bar sets the level.
    RowLayout {
        objectName: "audioFooter"
        Layout.fillWidth: true
        Layout.topMargin: 10
        Layout.preferredHeight: 28
        spacing: 8
        IslandButton {
            objectName: "microphoneMuteButton"
            text: island.micMuted ? "Unmute default microphone" : "Mute default microphone"
            symbol: island.micMuted ? "microphone-muted" : "microphone"
            implicitWidth: 28; implicitHeight: 28; glyphSize: 17
            visible: island.audioFeedback
            enabled: island.micAvailable
            onClicked: island.toggleMicMute()
        }
        IslandButton {
            text: island.muted ? "Unmute" : "Mute"
            symbol: island.muted ? "audio-volume-muted" : "audio-volume-high"
            implicitWidth: 28; implicitHeight: 28; glyphSize: 17
            enabled: island.audioAvailable
            onClicked: island.toggleMute()
        }
        Slider {
            id: volumeSlider
            objectName: "volumeSlider"
            // Custom visual items have no implicit size in the KDE style.
            implicitHeight: 24
            property int requestedVolume: 0
            Layout.fillWidth: true
            from: 0; to: 100
            enabled: island.audioAvailable
            opacity: enabled ? 1 : 0.45
            Accessible.name: "Output volume"
            onMoved: { requestedVolume = Math.round(value); volumeDebounce.restart() }
            Binding { target: volumeSlider; property: "value"; value: island.volume; when: !volumeSlider.pressed }
            Timer { id: volumeDebounce; interval: 100; onTriggered: island.setVolume(volumeSlider.requestedVolume) }
            background: Rectangle { x: volumeSlider.leftPadding; y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2; width: volumeSlider.availableWidth; height: 4; radius: 2; color: Theme.track; Rectangle { width: volumeSlider.visualPosition * parent.width; height: parent.height; radius: 2; color: Theme.accent } }
            handle: Rectangle { x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width); y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2; width: 10; height: 10; radius: 5; color: volumeSlider.enabled ? Theme.ink : Theme.secondary; border.width: volumeSlider.activeFocus ? 2 : 0; border.color: Theme.accent }
        }
        Text { textFormat: Text.PlainText; text: island.audioAvailable ? island.volume + "%" : "No audio"; color: view.secondary; font.pixelSize: 11; font.features: {"tnum": 1}; Layout.preferredWidth: 42; horizontalAlignment: Text.AlignRight }
    }
}
