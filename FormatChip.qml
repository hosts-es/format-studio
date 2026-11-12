import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string label: ""
    property string note: ""
    property bool selected: false

    signal clicked()

    implicitHeight: 46
    implicitWidth: Math.max(84, chipColumn.implicitWidth + 24)
    radius: 11
    color: root.selected ? Theme.accentSoft : (mouse.containsMouse ? Theme.surfaceHi : Theme.surfaceSunken)
    border.width: root.selected ? 1.4 : 1
    border.color: root.selected ? Theme.accent : Theme.border
    Behavior on color { ColorAnimation { duration: 120 } }
    Behavior on border.color { ColorAnimation { duration: 120 } }

    ColumnLayout {
        id: chipColumn
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: root.label
            color: root.selected ? Theme.accent : Theme.text
            font.pixelSize: 12
            font.weight: Font.DemiBold
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: root.note
            color: root.selected ? Theme.accent : Theme.textFaint
            opacity: root.selected ? 0.9 : 0.85
            font.pixelSize: 9
            Layout.alignment: Qt.AlignHCenter
            visible: text !== ""
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
