import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ScrollView {
    id: view
    clip: true
    contentWidth: availableWidth
    property var stats: island.statistics
    property var cpu: stats.cpu || ({})
    property var memory: stats.memory || ({})
    readonly property var temperatures: {
        let entries = [{label: "CPU", name: cpu.name || "Processor", icon: "cpu", temperature: cpu.temperature ?? -1}]
        let gpus = stats.gpus || []
        for (let i = 0; i < gpus.length; ++i)
            entries.push({label: gpus.length > 1 ? "GPU " + (i + 1) : "GPU", name: gpus[i].name, icon: "gpu", temperature: gpus[i].temperature ?? -1})
        if (!gpus.length)
            entries.push({label: "GPU", name: "Graphics adapter", icon: "gpu", temperature: -1})
        let drives = stats.nvme || []
        for (let i = 0; i < drives.length; ++i)
            entries.push({label: drives.length > 1 ? "NVMe " + (i + 1) : "NVMe", name: drives[i].name + " · " + drives[i].id, icon: "nvme", temperature: drives[i].temperature ?? -1})
        if (!drives.length)
            entries.push({label: "NVMe", name: "NVMe drive", icon: "nvme", temperature: -1})
        return entries
    }
    function bytes(n) {
        if (n === undefined || n < 0) return "Unavailable"
        if (n >= 1099511627776) return (n / 1099511627776).toFixed(2) + " TiB"
        if (n >= 1073741824) return (n / 1073741824).toFixed(1) + " GiB"
        return (n / 1048576).toFixed(0) + " MiB"
    }
    function percent(n) { return n !== undefined && n >= 0 ? Math.round(n) + "%" : "Unavailable" }
    ColumnLayout {
        width: view.availableWidth
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Text { textFormat: Text.PlainText; text: "System"; color: "#f5f5f7"; font.pixelSize: 20; font.weight: Font.DemiBold }
            Item { Layout.fillWidth: true }
            Rectangle { width: 5; height: 5; radius: 2.5; color: view.stats.ready ? "#9dd7b1" : "#a7a7af" }
            Text { textFormat: Text.PlainText; text: view.stats.ready ? "Live · CPU / RAM 1s" : "Reading system…"; color: "#a7a7af"; font.pixelSize: 11 }
        }
        GridLayout {
            objectName: "systemTemperatures"
            Layout.fillWidth: true
            columns: Math.min(4, view.temperatures.length)
            columnSpacing: 18
            rowSpacing: 12
            Repeater {
                model: view.temperatures
                TemperatureReading {
                    required property var modelData
                    Layout.fillWidth: true
                    label: modelData.label
                    deviceName: modelData.name
                    iconName: modelData.icon
                    temperature: modelData.temperature
                }
            }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#252529" }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 5
            RowLayout {
                Image { source: "qrc:/assets/icons/cpu.svg"; sourceSize.width: 14; sourceSize.height: 14; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
                Text { textFormat: Text.PlainText; text: "CPU"; color: "#f5f5f7"; font.pixelSize: 13; font.weight: Font.DemiBold }
                Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: view.cpu.name || "Processor"; color: "#a7a7af"; font.pixelSize: 12; elide: Text.ElideRight }
                Text { textFormat: Text.PlainText; text: view.cpu.ready ? view.percent(view.cpu.percent) : "Sampling…"; color: "#f5f5f7"; font.pixelSize: 20; font.weight: Font.Medium }
            }
            HistoryLine { Layout.fillWidth: true; Layout.preferredHeight: 24; samples: view.stats.cpuHistory || [] }
            Text { textFormat: Text.PlainText; text: (view.cpu.cores || "?") + " cores · " + (view.cpu.threads || "?") + " threads" + (view.cpu.mhz > 0 ? " · " + (view.cpu.mhz / 1000).toFixed(2) + " GHz" : ""); color: "#a7a7af"; font.pixelSize: 11 }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#252529" }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 5
            RowLayout {
                Image { source: "qrc:/assets/icons/ram.svg"; sourceSize.width: 14; sourceSize.height: 14; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
                Text { textFormat: Text.PlainText; text: "RAM"; color: "#f5f5f7"; font.pixelSize: 13; font.weight: Font.DemiBold }
                Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: view.memory.ready ? view.bytes(view.memory.used) + " / " + view.bytes(view.memory.total) + " used" : "Memory reading unavailable"; color: "#a7a7af"; font.pixelSize: 12 }
                Text { textFormat: Text.PlainText; text: view.memory.ready ? view.percent(view.memory.percent) : "—"; color: "#f5f5f7"; font.pixelSize: 20; font.weight: Font.Medium }
            }
            UsageTrack { Layout.fillWidth: true; value: view.memory.ready ? view.memory.percent : 0; tint: "#9dd7b1" }
            RowLayout {
                Text { textFormat: Text.PlainText; text: view.memory.ready ? view.bytes(view.memory.available) + " available" : "Requires /proc/meminfo"; color: "#9dd7b1"; font.pixelSize: 12 }
                Item { Layout.fillWidth: true }
                Text { textFormat: Text.PlainText; text: view.memory.swapTotal > 0 ? "Swap " + view.bytes(view.memory.swapUsed) + " / " + view.bytes(view.memory.swapTotal) : ""; color: "#a7a7af"; font.pixelSize: 11 }
            }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#252529" }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 12
            Repeater {
                model: view.stats.gpus || []
                ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true; spacing: 5
                    RowLayout {
                        Image { source: "qrc:/assets/icons/gpu.svg"; sourceSize.width: 14; sourceSize.height: 14; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
                        Text { textFormat: Text.PlainText; text: "GPU"; color: "#f5f5f7"; font.pixelSize: 13; font.weight: Font.DemiBold }
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.name; color: "#a7a7af"; font.pixelSize: 12; elide: Text.ElideRight; ToolTip.visible: gpuHover.hovered; ToolTip.text: text; HoverHandler { id: gpuHover } }
                        Text { textFormat: Text.PlainText; text: view.percent(modelData.percent); color: "#f5f5f7"; font.pixelSize: modelData.percent >= 0 ? 20 : 12; font.weight: Font.Medium }
                    }
                    UsageTrack { Layout.fillWidth: true; value: modelData.percent; tint: "#95bfff" }
                    RowLayout {
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.memoryTotal > 0 ? "VRAM " + view.bytes(modelData.memoryUsed) + " / " + view.bytes(modelData.memoryTotal) : "VRAM unavailable"; color: "#a7a7af"; font.pixelSize: 11 }
                        Text { textFormat: Text.PlainText; text: modelData.power >= 0 ? Math.round(modelData.power) + " W" : ""; color: "#a7a7af"; font.pixelSize: 11 }
                    }
                }
            }
            Text { textFormat: Text.PlainText; visible: !(view.stats.gpus || []).length; text: "No supported GPU telemetry found"; color: "#a7a7af"; font.pixelSize: 12 }
            Text { textFormat: Text.PlainText; visible: !!view.stats.gpuError; Layout.fillWidth: true; text: view.stats.gpuError || ""; wrapMode: Text.WordWrap; color: "#ffc078"; font.pixelSize: 11 }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#252529" }
        Repeater {
            model: view.stats.disks || []
            ColumnLayout {
                required property var modelData
                Layout.fillWidth: true; spacing: 5
                RowLayout {
                    Image { source: "qrc:/assets/icons/nvme.svg"; sourceSize.width: 14; sourceSize.height: 14; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
                    Text { textFormat: Text.PlainText; text: "Disk"; color: "#f5f5f7"; font.pixelSize: 13; font.weight: Font.DemiBold }
                    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.label; color: "#a7a7af"; font.pixelSize: 12; elide: Text.ElideRight }
                    Text { textFormat: Text.PlainText; text: view.percent(modelData.percent); color: "#f5f5f7"; font.pixelSize: 20; font.weight: Font.Medium }
                }
                UsageTrack { Layout.fillWidth: true; value: modelData.percent; tint: "#e8bf8b" }
                RowLayout {
                    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: view.bytes(modelData.used) + " / " + view.bytes(modelData.total) + " used"; color: "#a7a7af"; font.pixelSize: 12 }
                    Text { textFormat: Text.PlainText; text: view.bytes(modelData.available) + " free"; color: "#e8bf8b"; font.pixelSize: 12 }
                }
                Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.device + " · " + modelData.filesystem; color: "#a7a7af"; font.pixelSize: 11; elide: Text.ElideMiddle }
            }
        }
        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: "Temperatures 1s · GPU 2s · Disk 10s · RAM includes reclaimable cache"; color: "#a7a7af"; font.pixelSize: 10; wrapMode: Text.WordWrap }
    }
}
