import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    id: reading
    required property string label
    required property string deviceName
    required property string iconName
    property real temperature: -1
    readonly property bool available: isFinite(temperature) && temperature >= 0
    spacing: 5
    Accessible.role: Accessible.StaticText
    Accessible.name: deviceName + ": " + (available ? Math.round(temperature) + " degrees Celsius" : "temperature unavailable")
    RowLayout {
        spacing: 6
        Image { source: "qrc:/assets/icons/" + reading.iconName + ".svg"; sourceSize.width: 18; sourceSize.height: 18; Layout.preferredWidth: 18; Layout.preferredHeight: 18 }
        Text { text: reading.label; textFormat: Text.PlainText; color: "#a7a7af"; font.pixelSize: 11; font.weight: Font.Medium }
    }
    Text {
        textFormat: Text.PlainText
        text: reading.available ? Math.round(reading.temperature) + "°C" : "—"
        color: reading.available ? "#f5f5f7" : "#a7a7af"
        font.pixelSize: 22
        font.weight: Font.Medium
    }
    HoverHandler { id: hover }
    ToolTip.visible: hover.hovered
    ToolTip.text: reading.deviceName + (reading.available ? "" : " · Temperature unavailable")
}
