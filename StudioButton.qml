import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string text: ""
    property string icon: ""
    property bool primary: false
    property bool danger: false
    property bool compact: false

    signal clicked()

    implicitHeight: compact ? 32 : 40
    implicitWidth: content.implicitWidth + (compact ? 22 : 34)
    opacity: enabled ? 1 : 0.4

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusSm
        color: {
            if (root.primary)
                return mouse.containsMouse && root.enabled ? Theme.accentHover : Theme.accent
            if (root.danger)
                return mouse.containsMouse && root.enabled ? Theme.dangerSoft : "transparent"
            return mouse.containsMouse && root.enabled ? Theme.surfaceSunken : "transparent"
        }
        border.width: (root.primary || root.danger) ? 0 : 1
        border.color: Theme.border
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    RowLayout {
        id: content
        anchors.centerIn: parent
        spacing: (root.text !== "" && root.icon !== "") ? 7 : 0

        Icon {
            name: root.icon
            visible: root.icon !== ""
            size: root.compact ? 14 : 16
            color: root.primary ? Theme.accentText : (root.danger ? Theme.danger : Theme.text)
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            text: root.text
            visible: root.text !== ""
            color: root.primary ? Theme.accentText : (root.danger ? Theme.danger : Theme.text)
            font.pixelSize: root.compact ? 12 : 13
            font.weight: root.primary ? Font.DemiBold : Font.Normal
            Layout.alignment: Qt.AlignVCenter
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
