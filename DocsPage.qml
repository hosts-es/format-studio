import QtQuick
import QtQuick.Layouts

Item {
    id: page

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 80)
        spacing: 16

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 84
            height: 84
            radius: 26
            color: Theme.surfaceSunken
            border.width: 1
            border.color: Theme.border

            Icon {
                anchors.centerIn: parent
                name: "lock"
                size: 34
                color: Theme.textFaint
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "文档转换 · 规划中"
            color: Theme.text
            font.pixelSize: 20
            font.weight: Font.Bold
        }

        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: "这部分会在 v0.3 与视频 / 音频 / 图片统一到同一条队列里，计划接入 LibreOffice 无头模式完成 Office 与 PDF 互转。"
            color: Theme.textDim
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            lineHeight: 1.4
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 8
            implicitHeight: planColumn.implicitHeight + 40
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.border

            ColumnLayout {
                id: planColumn
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                Text {
                    text: "计划中的能力"
                    color: Theme.text
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                Repeater {
                    model: [
                        { "icon": "doc", "title": "Word / Excel / PPT → PDF", "desc": "保留版式与字体，批量转换" },
                        { "icon": "layers", "title": "PDF 拆分与合并", "desc": "按页提取、加密与压缩" },
                        { "icon": "image", "title": "文档转图片", "desc": "长图、缩略图与 OCR 文本提取" }
                    ]

                    delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Icon {
                            Layout.alignment: Qt.AlignVCenter
                            name: modelData.icon
                            size: 16
                            color: Theme.textDim
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                text: modelData.title
                                color: Theme.text
                                font.pixelSize: 12
                            }

                            Text {
                                text: modelData.desc
                                color: Theme.textFaint
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }
        }
    }
}
