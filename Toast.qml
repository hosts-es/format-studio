import QtQuick
import QtQuick.Layouts

Item {
    id: root

    width: 340
    height: column.implicitHeight

    function show(text, level) {
        if (!text)
            return
        listModel.append({ "message": text, "level": level || "info" })
        while (listModel.count > 4)
            listModel.remove(0)
    }

    ListModel { id: listModel }

    Behavior on height { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

    ColumnLayout {
        id: column
        width: root.width
        spacing: 8

        Repeater {
            model: listModel

            delegate: Rectangle {
                id: toastItem

                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(46, messageLabel.implicitHeight + 24)
                radius: 12
                color: {
                    switch (level) {
                    case "success": return Theme.successSoft
                    case "error": return Theme.dangerSoft
                    case "warning": return Theme.warningSoft
                    default: return Theme.surfaceHi
                    }
                }
                border.width: 1
                border.color: Theme.border

                opacity: 0
                scale: 0.96
                Component.onCompleted: {
                    opacity = 1
                    scale = 1
                }
                Behavior on opacity { NumberAnimation { duration: 180 } }
                Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 10

                    Icon {
                        Layout.alignment: Qt.AlignVCenter
                        name: level === "success" ? "check"
                              : level === "error" ? "alert"
                              : level === "warning" ? "alert" : "info"
                        size: 17
                        color: level === "success" ? Theme.success
                               : level === "error" ? Theme.danger
                               : level === "warning" ? Theme.warning : Theme.textDim
                    }

                    Text {
                        id: messageLabel
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        text: message
                        color: Theme.text
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }

                Timer {
                    interval: 3200
                    running: true
                    onTriggered: listModel.remove(index)
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: listModel.remove(index)
                }
            }
        }
    }
}
