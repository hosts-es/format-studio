import QtQuick
import QtQuick.Controls

Item {
    id: root

    signal clicked()

    implicitHeight: 62
    implicitWidth: Theme.sidebarWidth - 18

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: mouse.containsMouse ? Theme.surfaceHi : "transparent"
        Behavior on color { ColorAnimation { duration: 140 } }
    }

    Icon {
        id: iconItem

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 11
        name: Theme.dark ? "moon" : "sun"
        size: 21
        color: mouse.containsMouse ? Theme.text : Theme.textDim
        Behavior on color { ColorAnimation { duration: 140 } }

        onNameChanged: spin.restart()
    }

    NumberAnimation {
        id: spin
        target: iconItem
        property: "rotation"
        from: -40
        to: 0
        duration: 260
        easing.type: Easing.OutCubic
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: iconItem.bottom
        anchors.topMargin: 5
        text: Theme.dark ? "深色" : "浅色"
        color: mouse.containsMouse ? Theme.text : Theme.textDim
        font.pixelSize: 11
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip.visible: mouse.containsMouse
    ToolTip.text: Theme.dark ? "切换到浅色主题" : "切换到深色主题"
    ToolTip.delay: 500
}
