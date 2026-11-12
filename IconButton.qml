import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string icon: ""
    property string tooltip: ""
    property color iconColor: Theme.textDim
    property color hoverColor: Theme.surfaceSunken
    property int size: 30
    property int iconSize: 16

    signal clicked()

    implicitWidth: size
    implicitHeight: size
    opacity: enabled ? 1 : 0.35

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: mouse.containsMouse && root.enabled ? root.hoverColor : "transparent"
        Behavior on color { ColorAnimation { duration: 110 } }
    }

    Icon {
        anchors.centerIn: parent
        name: root.icon
        size: root.iconSize
        color: mouse.containsMouse && root.enabled ? Theme.text : root.iconColor
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip.visible: root.tooltip !== "" && mouse.containsMouse
    ToolTip.text: root.tooltip
    ToolTip.delay: 500
}
