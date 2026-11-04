import QtQuick
Canvas {
    property var samples: []
    property color tint: Theme.accent
    onSamplesChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        let ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        if (samples.length < 2) return
        ctx.strokeStyle = tint; ctx.lineWidth = 1.5; ctx.lineJoin = "round"
        ctx.beginPath()
        for (let i = 0; i < samples.length; ++i) {
            let x = width * i / Math.max(59, samples.length - 1)
            let y = height - 2 - Math.max(0, Math.min(100, samples[i])) / 100 * (height - 4)
            if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
        }
        ctx.stroke()
    }
}
