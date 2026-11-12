import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property var options: []
    property int currentIndex: 0

    signal activated(int index)

    implicitHeight: 36
    implicitWidth: row.implicitWidth + 10
    radius: 10
    color: Theme.surfaceSunken
    border.width: 1
    border.color: Theme.border

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 2

        Repeater {
            model: root.options

            delegate: Rectangle {
                id: segment

                Layout.preferredHeight: 28
                Layout.preferredWidth: label.implicitWidth + 26
                radius: 8
                color: index === root.currentIndex ? Theme.surface : "transparent"
                border.width: index === root.currentIndex ? 1 : 0
                border.color: Theme.border
                Behavior on color { ColorAnimation { duration: 120 } }

                Text {
                    id: label
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 12
                    font.weight: index === root.currentIndex ? Font.DemiBold : Font.Normal
                    color: index === root.currentIndex ? Theme.text : Theme.textDim
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.activated(index)
                }
            }
        }
    }
}
