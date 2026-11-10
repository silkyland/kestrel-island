import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: view
    // Glanceable cards up front; the previous detailed rows stay behind a
    // disclosure that only lives in memory (decision 6).
    property bool details: false
    property var stats: island.statistics
    property var cpu: stats.cpu || ({})
    property var memory: stats.memory || ({})
    function bytes(n) {
        if (n === undefined || n < 0) return "Unavailable"
        if (n >= 1099511627776) return (n / 1099511627776).toFixed(2) + " TiB"
        if (n >= 1073741824) return (n / 1073741824).toFixed(1) + " GiB"
        return (n / 1048576).toFixed(0) + " MiB"
    }
    function percent(n) { return n !== undefined && n >= 0 ? Math.round(n) + "%" : "Unavailable" }
    function tempText(t) { return t >= 0 ? " · " + Math.round(t) + "°C" : "" }
    readonly property var cards: {
        const list = []
        const c = cpu
        list.push({title: "CPU", symbol: "cpu",
            value: c.ready ? Math.round(c.percent) + "%" : "Sampling…",
            subtitle: (c.name || "Processor") + tempText(c.temperature ?? -1),
            meter: c.ready ? Math.max(0, c.percent) : 0, meterTint: Theme.accent,
            attention: (c.percent ?? 0) >= 85})
        const m = memory
        list.push({title: "RAM", symbol: "ram",
            value: m.ready ? Math.round(m.percent) + "%" : "—",
            subtitle: m.ready ? bytes(m.used) + " / " + bytes(m.total) + " used" : "Reading unavailable",
            meter: m.ready ? m.percent : 0, meterTint: Theme.ok,
            attention: (m.percent ?? 0) >= 85})
        const gpus = stats.gpus || []
        if (gpus.length) {
            for (let i = 0; i < gpus.length; ++i) {
                const g = gpus[i]
                list.push({title: gpus.length > 1 ? "GPU " + (i + 1) : "GPU", symbol: "gpu",
                    value: g.percent >= 0 ? Math.round(g.percent) + "%" : "—",
                    subtitle: (g.name || "Graphics adapter") + tempText(g.temperature ?? -1),
                    meter: g.percent >= 0 ? g.percent : 0, meterTint: Theme.info,
                    attention: (g.percent ?? -1) >= 85})
            }
        } else {
            list.push({title: "GPU", symbol: "gpu", value: "—",
                subtitle: "No supported GPU telemetry",
                meter: -1, meterTint: Theme.info, attention: false})
        }
        const disks = stats.disks || []
        const nvme = stats.nvme || []
        for (let i = 0; i < disks.length; ++i) {
            const d = disks[i]
            const free = d.total > 0 ? d.available / d.total : 1
            list.push({title: disks.length > 1 ? "Disk " + (i + 1) : "Disk", symbol: "nvme",
                value: Math.round(d.percent) + "%",
                subtitle: (d.label || "Disk") + " · " + bytes(d.available) + " free" + (i < nvme.length ? tempText(nvme[i].temperature ?? -1) : ""),
                meter: d.percent >= 0 ? d.percent : 0, meterTint: Theme.secondary,
                attention: free < 0.10})
        }
        for (let i = disks.length; i < nvme.length; ++i) {
            const n = nvme[i]
            list.push({title: nvme.length > 1 ? "NVMe " + (i + 1) : "NVMe", symbol: "nvme",
                value: n.temperature >= 0 ? Math.round(n.temperature) + "°C" : "—",
                subtitle: n.name || "NVMe drive",
                meter: -1, meterTint: Theme.secondary, attention: false})
        }
        return list
    }
    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
            width: scroll.availableWidth
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                Text { textFormat: Text.PlainText; text: "System"; color: Theme.ink; font.pixelSize: 20; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
                Rectangle { width: 5; height: 5; radius: 2.5; color: view.stats.ready ? Theme.ok : Theme.secondary }
                Text { textFormat: Text.PlainText; text: view.stats.ready ? "Live · CPU / RAM 1s" : "Reading system…"; color: Theme.secondary; font.pixelSize: 11 }
            }
            GridLayout {
                objectName: "systemTemperatures"
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 10
                rowSpacing: 10
                Repeater {
                    // The card count, not the freshly built card array: the
                    // delegates then keep their identity as the 1 s sample
                    // arrives and their meters animate instead of flashing.
                    model: view.cards.length
                    MetricCard {
                        required property int index
                        readonly property var card: view.cards[index] || ({})
                        Layout.fillWidth: true
                        Layout.columnSpan: view.cards.length % 2 === 1 && index === view.cards.length - 1 ? 2 : 1
                        title: card.title || ""
                        symbol: card.symbol || ""
                        value: card.value || ""
                        subtitle: card.subtitle || ""
                        meter: card.meter ?? -1
                        meterTint: card.meterTint || Theme.accent
                        attention: card.attention === true
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 2
                Text { textFormat: Text.PlainText; text: "DETAILS"; color: Theme.secondary; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 1 }
                Item { Layout.fillWidth: true }
                IslandAction { text: view.details ? "Hide details" : "Details"; checked: view.details; onClicked: view.details = !view.details }
            }
            ColumnLayout {
                Layout.fillWidth: true
                visible: view.details
                spacing: 10
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    RowLayout {
                        Image { source: "qrc:/assets/icons/cpu.svg"; sourceSize.width: 14; sourceSize.height: 14; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
                        Text { textFormat: Text.PlainText; text: "CPU"; color: Theme.ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: view.cpu.name || "Processor"; color: Theme.secondary; font.pixelSize: 12; elide: Text.ElideRight }
                        Text { textFormat: Text.PlainText; text: view.cpu.ready ? view.percent(view.cpu.percent) : "Sampling…"; color: Theme.ink; font.pixelSize: 20; font.weight: Font.Medium }
                    }
                    HistoryLine { Layout.fillWidth: true; Layout.preferredHeight: 24; samples: view.stats.cpuHistory || [] }
                    Text { textFormat: Text.PlainText; text: (view.cpu.cores || "?") + " cores · " + (view.cpu.threads || "?") + " threads" + (view.cpu.mhz > 0 ? " · " + (view.cpu.mhz / 1000).toFixed(2) + " GHz" : ""); color: Theme.secondary; font.pixelSize: 11 }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    RowLayout {
                        Image { source: "qrc:/assets/icons/ram.svg"; sourceSize.width: 14; sourceSize.height: 14; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
                        Text { textFormat: Text.PlainText; text: "RAM"; color: Theme.ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: view.memory.ready ? view.bytes(view.memory.used) + " / " + view.bytes(view.memory.total) + " used" : "Memory reading unavailable"; color: Theme.secondary; font.pixelSize: 12 }
                        Text { textFormat: Text.PlainText; text: view.memory.ready ? view.percent(view.memory.percent) : "—"; color: Theme.ink; font.pixelSize: 20; font.weight: Font.Medium }
                    }
                    UsageTrack { Layout.fillWidth: true; value: view.memory.ready ? view.memory.percent : 0; tint: Theme.ok }
                    RowLayout {
                        Text { textFormat: Text.PlainText; text: view.memory.ready ? view.bytes(view.memory.available) + " available" : "Requires /proc/meminfo"; color: Theme.ok; font.pixelSize: 12 }
                        Item { Layout.fillWidth: true }
                        Text { textFormat: Text.PlainText; text: view.memory.swapTotal > 0 ? "Swap " + view.bytes(view.memory.swapUsed) + " / " + view.bytes(view.memory.swapTotal) : ""; color: Theme.secondary; font.pixelSize: 11 }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 10
                    Repeater {
                        model: view.stats.gpus || []
                        ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true; spacing: 5
                            RowLayout {
                                Image { source: "qrc:/assets/icons/gpu.svg"; sourceSize.width: 14; sourceSize.height: 14; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
                                Text { textFormat: Text.PlainText; text: "GPU"; color: Theme.ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                                Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.name; color: Theme.secondary; font.pixelSize: 12; elide: Text.ElideRight; ToolTip.visible: gpuHover.hovered; ToolTip.text: text; HoverHandler { id: gpuHover } }
                                Text { textFormat: Text.PlainText; text: view.percent(modelData.percent); color: Theme.ink; font.pixelSize: modelData.percent >= 0 ? 20 : 12; font.weight: Font.Medium }
                            }
                            UsageTrack { Layout.fillWidth: true; value: modelData.percent; tint: Theme.info }
                            RowLayout {
                                Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.memoryTotal > 0 ? "VRAM " + view.bytes(modelData.memoryUsed) + " / " + view.bytes(modelData.memoryTotal) : "VRAM unavailable"; color: Theme.secondary; font.pixelSize: 11 }
                                Text { textFormat: Text.PlainText; text: modelData.power >= 0 ? Math.round(modelData.power) + " W" : ""; color: Theme.secondary; font.pixelSize: 11 }
                            }
                        }
                    }
                    Text { textFormat: Text.PlainText; visible: !(view.stats.gpus || []).length; text: "No supported GPU telemetry found"; color: Theme.secondary; font.pixelSize: 12 }
                    Text { textFormat: Text.PlainText; visible: !!view.stats.gpuError; Layout.fillWidth: true; text: view.stats.gpuError || ""; wrapMode: Text.WordWrap; color: Theme.timer; font.pixelSize: 11 }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }
                Repeater {
                    model: view.stats.disks || []
                    ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true; spacing: 5
                        RowLayout {
                            Image { source: "qrc:/assets/icons/nvme.svg"; sourceSize.width: 14; sourceSize.height: 14; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
                            Text { textFormat: Text.PlainText; text: "Disk"; color: Theme.ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                            Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.label; color: Theme.secondary; font.pixelSize: 12; elide: Text.ElideRight }
                            Text { textFormat: Text.PlainText; text: view.percent(modelData.percent); color: Theme.ink; font.pixelSize: 20; font.weight: Font.Medium }
                        }
                        UsageTrack { Layout.fillWidth: true; value: modelData.percent; tint: Theme.warn }
                        RowLayout {
                            Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: view.bytes(modelData.used) + " / " + view.bytes(modelData.total) + " used"; color: Theme.secondary; font.pixelSize: 12 }
                            Text { textFormat: Text.PlainText; text: view.bytes(modelData.available) + " free"; color: Theme.warn; font.pixelSize: 12 }
                        }
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.device + " · " + modelData.filesystem; color: Theme.secondary; font.pixelSize: 11; elide: Text.ElideMiddle }
                    }
                }
                Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: "Temperatures 1s · GPU 2s · Disk 10s · RAM includes reclaimable cache"; color: Theme.secondary; font.pixelSize: 10; wrapMode: Text.WordWrap }
            }
        }
    }
    EdgeFade { anchors.fill: parent; target: scroll.contentItem }
}
