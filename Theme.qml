pragma Singleton
import QtQuick
import QtQuick.Controls

QtObject {
    id: theme

    readonly property bool dark: {
        if (backend.themeMode === "dark")
            return true
        if (backend.themeMode === "light")
            return false
        return Application.styleHints.colorScheme === Qt.Dark
    }

    // ── surfaces ──────────────────────────────────────────────
    readonly property color bg: dark ? "#0C0E14" : "#F2F5FA"
    readonly property color sidebar: dark ? "#11141B" : "#FFFFFF"
    readonly property color surface: dark ? "#161A23" : "#FFFFFF"
    readonly property color surfaceHi: dark ? "#1D2330" : "#F7F9FD"
    readonly property color surfaceSunken: dark ? "#0F1218" : "#EEF2F9"
    readonly property color border: dark ? "#242B38" : "#E4E9F2"
    readonly property color borderStrong: dark ? "#333D4F" : "#CBD4E4"

    // ── text ──────────────────────────────────────────────────
    readonly property color text: dark ? "#EEF1F7" : "#111725"
    readonly property color textDim: dark ? "#A3AEC1" : "#59647A"
    readonly property color textFaint: dark ? "#6B7688" : "#8B95A8"

    // ── accents ───────────────────────────────────────────────
    readonly property color accent: dark ? "#5B8DEF" : "#2F6BFF"
    readonly property color accentHover: dark ? "#7AA6FF" : "#1F5AF0"
    readonly property color accentSoft: dark ? "#1A2438" : "#E9F0FF"
    readonly property color accentText: "#FFFFFF"

    readonly property color success: dark ? "#3BD98F" : "#0E9F5B"
    readonly property color successSoft: dark ? "#11291F" : "#E4F7ED"
    readonly property color warning: dark ? "#F3B742" : "#B57300"
    readonly property color warningSoft: dark ? "#2A2213" : "#FFF4DF"
    readonly property color danger: dark ? "#FF7373" : "#D63A3A"
    readonly property color dangerSoft: dark ? "#2B171A" : "#FDEBEB"

    // ── geometry ──────────────────────────────────────────────
    readonly property int radius: 16
    readonly property int radiusSm: 10
    readonly property int radiusXs: 7
    readonly property int sidebarWidth: 92

    readonly property string fontFamily: "Microsoft YaHei UI"

    function formatSize(bytes) {
        if (!bytes || bytes <= 0)
            return "—"
        var units = ["B", "KB", "MB", "GB", "TB"]
        var value = bytes
        var index = 0
        while (value >= 1024 && index < units.length - 1) {
            value /= 1024
            ++index
        }
        return (index === 0 ? value.toFixed(0) : value.toFixed(value < 10 ? 1 : 0)) + " " + units[index]
    }

    function formatClock(ms) {
        if (!ms || ms <= 0)
            return "—"
        var total = Math.floor(ms / 1000)
        var h = Math.floor(total / 3600)
        var m = Math.floor((total % 3600) / 60)
        var s = total % 60
        function pad(n) { return (n < 10 ? "0" : "") + n }
        return h > 0 ? h + ":" + pad(m) + ":" + pad(s) : pad(m) + ":" + pad(s)
    }

    function formatEta(ms) {
        if (ms === undefined || ms < 0)
            return ""
        var total = Math.max(0, Math.round(ms / 1000))
        if (total < 60)
            return "剩余 " + total + " 秒"
        var m = Math.floor(total / 60)
        var s = total % 60
        return "剩余 " + m + ":" + (s < 10 ? "0" : "") + s
    }
}
