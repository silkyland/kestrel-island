import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string symbol: ""
    property bool prominent: false
    property bool selected: false
    property bool dim: false
    implicitWidth: 40
    implicitHeight: 40
    padding: 8
    icon.source: "qrc:/assets/icons/" + symbol + ".svg"
    icon.width: 20
    icon.height: 20
    display: AbstractButton.IconOnly
    Accessible.name: text
    // Render our bundled SVG directly: KDE's platform Button delegate may
    // ignore icon.source when icon.name is empty.
    contentItem: Image {
        objectName: "buttonIcon"
        source: control.icon.source
        sourceSize.width: 20
        sourceSize.height: 20
        fillMode: Image.PreserveAspectFit
        opacity: !control.enabled ? 0.3 : control.dim && !control.selected && !control.hovered ? 0.55 : 1
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignVCenter
    }
    background: Rectangle {
        radius: width / 2
        color: control.down ? "#45454d" : control.checked || control.selected ? "#3b324f" : control.hovered || control.activeFocus ? "#333339" : control.prominent ? "#242427" : "transparent"
        border.width: control.activeFocus ? 2 : control.checked || control.selected ? 1 : 0
        border.color: "#b8a1ff"
    }
    ToolTip.visible: hovered && text.length > 0
    ToolTip.text: text
    ToolTip.delay: 650
}
