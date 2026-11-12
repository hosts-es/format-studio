import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string title: "拖放文件到这里"
    property string subtitle: "支持视频 / 音频 / 图片；可以一次拖入多个文件，或直接拖入整个文件夹"
    property bool compact: false

    signal filesDropped(var urls)
    signal clicked()

    implicitHeight: compact ? 84 : 128
    radius: Theme.radius
    color: drop.containsDrag ? Theme.accentSoft : (mouse.containsMouse ? Theme.surfaceHi : Theme.surface)
    border.width: drop.containsDrag ? 2 : 1
    border.color: drop.containsDrag ? Theme.accent : Theme.border

    Behavior on color { ColorAnimation { duration: 150 } }
    Behavior on border.color { ColorAnimation { duration: 150 } }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 18

        Rectangle {
            Layout.preferredWidth: root.compact ? 44 : 52
            Layout.preferredHeight: root.compact ? 44 : 52
            radius: 14
            color: drop.containsDrag ? Theme.accent : Theme.accentSoft

            Icon {
                anchors.centerIn: parent
                name: "download"
                size: root.compact ? 20 : 24
                color: drop.containsDrag ? Theme.accentText : Theme.accent
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3

            Text {
                text: root.title
                color: Theme.text
                font.pixelSize: root.compact ? 14 : 15
                font.weight: Font.DemiBold
            }

            Text {
                Layout.fillWidth: true
                text: root.subtitle
                color: Theme.textDim
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                visible: !root.compact
            }
        }

        StudioButton {
            text: "选择文件"
            icon: "plus"
            primary: !root.compact
            onClicked: root.clicked()
        }
    }

    DropArea {
        id: drop
        anchors.fill: parent
        onDropped: (drag) => root.filesDropped(drag.urls)
    }
}
