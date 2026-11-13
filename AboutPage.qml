import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: page

    signal requestPage(string name)

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: column.implicitHeight + 52
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: column
            width: Math.min(parent.width - 52, 720)
            x: Math.max(26, (parent.width - width) / 2)
            y: 26
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: bodyColumn.implicitHeight + 56
                radius: Theme.radius
                color: Theme.surface
                border.width: 1
                border.color: Theme.border

                ColumnLayout {
                    id: bodyColumn
                    anchors.fill: parent
                    anchors.margins: 28
                    spacing: 18

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 18

                        Image {
                            width: 64
                            height: 64
                            source: "qrc:/icons/app.png"
                            sourceSize.width: 128
                            sourceSize.height: 128
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }

                        ColumnLayout {
                            spacing: 3

                            Text {
                                text: "Format Studio"
                                color: Theme.text
                                font.pixelSize: 22
                                font.weight: Font.Bold
                            }

                            Text {
                                text: "V" + updates.currentVersion
                                color: Theme.textDim
                                font.pixelSize: 12
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "基于 QT 和 FFmpeg 的多功能格式转换器"
                        color: Theme.textDim
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        lineHeight: 1.35
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            text: "作者"
                            color: Theme.textDim
                            font.pixelSize: 12
                        }

                        Icon {
                            name: "github"
                            size: 16
                            color: Theme.text
                        }

                        Text {
                            text: "hosts-es"
                            color: Theme.accent
                            font.pixelSize: 12
                            font.underline: authorMouse.containsMouse
                            MouseArea {
                                id: authorMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: updates.openUrl("https://github.com/hosts-es")
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        StudioButton {
                            text: updates.checking ? "检查中…" : "检查更新"
                            icon: "refresh"
                            enabled: !updates.checking && !updates.downloading
                            onClicked: updates.checkForUpdates()
                        }

                        StudioButton {
                            text: updates.downloading
                                  ? "下载中 " + updates.downloadPercent + "%"
                                  : "提交日志反馈"
                            icon: "doc"
                            enabled: !updates.downloading
                            onClicked: page.requestPage("feedback")
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            visible: updates.updateAvailable && !updates.downloading
                            text: "发现新版本 V" + updates.latestVersion
                            color: Theme.accent
                            font.pixelSize: 11
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: "Format Studio · 2026"
                color: Theme.textFaint
                font.pixelSize: 11
            }

            Item { Layout.preferredHeight: 8 }
        }
    }
}
