import QtQuick

Item {
    id: root

    property string label: ""
    property string icon: ""
    property bool selected: false
    property bool soon: false
    property int badge: 0

    signal clicked()

    implicitHeight: 62
    implicitWidth: Theme.sidebarWidth - 18

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: root.selected ? Theme.accentSoft : (mouse.containsMouse ? Theme.surfaceHi : "transparent")
        Behavior on color { ColorAnimation { duration: 140 } }
    }

    Rectangle {
        width: 3
        height: 20
        radius: 2
        color: Theme.accent
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 1
        opacity: root.selected ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 140 } }
    }

    Icon {
        id: iconItem
        name: root.icon
        size: 21
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 11
        color: root.selected ? Theme.accent : (mouse.containsMouse ? Theme.text : Theme.textDim)
        Behavior on color { ColorAnimation { duration: 140 } }
    }

    Text {
        text: root.label
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: iconItem.bottom
        anchors.topMargin: 5
        color: root.selected ? Theme.text : Theme.textDim
        font.pixelSize: 11
        font.weight: root.selected ? Font.DemiBold : Font.Normal
    }

    Rectangle {
        visible: root.badge > 0
        width: Math.max(18, badgeLabel.implicitWidth + 10)
        height: 17
        radius: 9
        color: Theme.accent
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 6
        anchors.rightMargin: 7

        Text {
            id: badgeLabel
            anchors.centerIn: parent
            text: String(root.badge)
            color: Theme.accentText
            font.pixelSize: 10
            font.bold: true
        }
    }

    Rectangle {
        visible: root.soon
        width: soonLabel.implicitWidth + 10
        height: 16
        radius: 8
        color: Theme.surfaceSunken
        border.width: 1
        border.color: Theme.border
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 7
        anchors.rightMargin: 6

        Text {
            id: soonLabel
            anchors.centerIn: parent
            text: "soon"
            color: Theme.textFaint
            font.pixelSize: 9
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
