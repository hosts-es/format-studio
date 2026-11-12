import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string icon: "layers"
    property string title: ""
    property string subtitle: ""

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 76
            Layout.preferredHeight: 76
            radius: 24
            color: Theme.surfaceSunken
            border.width: 1
            border.color: Theme.border

            Icon {
                anchors.centerIn: parent
                name: root.icon
                size: 32
                color: Theme.textFaint
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.title
            color: Theme.text
            font.pixelSize: 15
            font.weight: Font.DemiBold
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.subtitle
            color: Theme.textDim
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
