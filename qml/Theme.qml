pragma Singleton
import QtQuick

QtObject {
    readonly property color surface: "#080809"
    readonly property color ink: "#f5f5f7"
    readonly property color secondary: "#a7a7af"
    readonly property color faint: "#777780"
    readonly property color raised: "#202023"
    readonly property color raisedHover: "#2b2b30"
    readonly property color raisedPressed: "#3a3a42"
    readonly property color divider: "#252529"
    readonly property color track: "#303035"
    readonly property color accent: "#b8a1ff"
    readonly property color accentInk: "#16121e"
    readonly property color timer: "#f5a524"
    readonly property color ok: "#9dd7b1"
    readonly property color info: "#95bfff"
    readonly property color warn: "#e8bf8b"
    readonly property color selectedFill: Qt.rgba(1, 1, 1, 0.12)
    readonly property color hairline: Qt.rgba(1, 1, 1, 0.12)

    function luminance(c) {
        const f = function(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
        return 0.2126 * f(c.r) + 0.7152 * f(c.g) + 0.0722 * f(c.b)
    }
    function contrast(a, b) {
        const x = luminance(Qt.color(a)) + 0.05
        const y = luminance(Qt.color(b)) + 0.05
        return x > y ? x / y : y / x
    }
    function dur(ms) { return island.animations ? Math.round(ms * island.animationScale) : 0 }

    // Cover tint drives the parts that belong to the current track; neutral
    // or low-contrast covers keep the default accent.
    readonly property color tint: {
        const raw = String(island.media.tint || "")
        if (!raw.length || raw === "#000000")
            return accent
        const c = Qt.color(raw)
        return contrast(c, surface) >= 4.5 ? c : accent
    }
}
