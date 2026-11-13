import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

Item {
    id: page

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.implicitHeight + 52
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: contentColumn
            x: 26
            y: 26
            width: parent.width - 52
            spacing: 16

            Text {
                text: "设置"
                color: Theme.text
                font.pixelSize: 26
                font.weight: Font.Bold
            }

            Text {
                text: "外观、转换默认值与运行环境"
                color: Theme.textDim
                font.pixelSize: 12
            }

            // ── 外观 ──────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: appearanceColumn.implicitHeight + 36
                radius: Theme.radius
                color: Theme.surface
                border.width: 1
                border.color: Theme.border

                ColumnLayout {
                    id: appearanceColumn
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 16

                    Text {
                        text: "外观"
                        color: Theme.text
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: "主题"
                                color: Theme.text
                                font.pixelSize: 13
                            }

                            Text {
                                text: "默认跟随系统深浅色，可手动固定"
                                color: Theme.textDim
                                font.pixelSize: 11
                            }
                        }

                        Segmented {
                            options: ["跟随系统", "浅色", "深色"]
                            currentIndex: backend.themeMode === "light" ? 1
                                          : backend.themeMode === "dark" ? 2 : 0
                            onActivated: (index) => backend.themeMode = index === 0 ? "auto"
                                                                      : index === 1 ? "light" : "dark"
                        }
                    }
                }
            }

            // ── 转换默认值 ────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: convertColumn.implicitHeight + 36
                radius: Theme.radius
                color: Theme.surface
                border.width: 1
                border.color: Theme.border

                ColumnLayout {
                    id: convertColumn
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 16

                    Text {
                        text: "转换默认值"
                        color: Theme.text
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: "画质预设"
                                color: Theme.text
                                font.pixelSize: 13
                            }

                            Text {
                                text: "控制视频 CRF 与音频比特率"
                                color: Theme.textDim
                                font.pixelSize: 11
                            }
                        }

                        Segmented {
                            options: ["高画质", "均衡", "小体积"]
                            currentIndex: backend.quality
                            onActivated: (index) => backend.quality = index
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: "同时转换任务数"
                                color: Theme.text
                                font.pixelSize: 13
                            }

                            Text {
                                text: "机器性能越好可以开得越多，机械硬盘建议 1–2"
                                color: Theme.textDim
                                font.pixelSize: 11
                            }
                        }

                        Segmented {
                            options: ["1", "2", "3", "4"]
                            currentIndex: backend.concurrency - 1
                            onActivated: (index) => backend.concurrency = index + 1
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: "同名文件处理"
                                color: Theme.text
                                font.pixelSize: 13
                            }

                            Text {
                                text: "输出目录已存在同名文件时的行为"
                                color: Theme.textDim
                                font.pixelSize: 11
                            }
                        }

                        Segmented {
                            options: ["自动重命名", "覆盖", "跳过"]
                            currentIndex: backend.overwritePolicy
                            onActivated: (index) => backend.overwritePolicy = index
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        ColumnLayout {
                            spacing: 2

                            Text {
                                text: "输出文件夹"
                                color: Theme.text
                                font.pixelSize: 13
                            }

                            Text {
                                text: "转换结果保存的位置"
                                color: Theme.textDim
                                font.pixelSize: 11
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            radius: Theme.radiusSm
                            color: Theme.surfaceSunken
                            border.width: 1
                            border.color: Theme.border

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 8
                                spacing: 10

                                Icon {
                                    name: "folder"
                                    size: 15
                                    color: Theme.textDim
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: backend.outputDir
                                    color: Theme.text
                                    font.pixelSize: 12
                                    elide: Text.ElideMiddle
                                }

                                StudioButton {
                                    text: "更改"
                                    compact: true
                                    onClicked: folderDialog.open()
                                }

                                StudioButton {
                                    text: "打开"
                                    compact: true
                                    onClicked: backend.openOutputDir()
                                }
                            }
                        }
                    }
                }
            }

            // ── 引擎 ──────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: engineColumn.implicitHeight + 36
                radius: Theme.radius
                color: Theme.surface
                border.width: 1
                border.color: Theme.border

                ColumnLayout {
                    id: engineColumn
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12

                    Text {
                        text: "前置环境检测"
                        color: Theme.text
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            width: 9
                            height: 9
                            radius: 4.5
                            color: backend.ffmpegReady ? Theme.success : Theme.danger
                        }

                        Text {
                            Layout.fillWidth: true
                            text: backend.ffmpegReady ? "转换组件已就绪" : "未检测到转换组件"
                            color: Theme.text
                            font.pixelSize: 13
                        }

                        StudioButton {
                            text: "重新检测"
                            icon: "refresh"
                            compact: true
                            onClicked: backend.detectFfmpeg()
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: !backend.ffmpegReady
                        text: "转换组件缺失，转换功能将不可用。建议重新安装 Format Studio。"
                        color: Theme.textDim
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // ── 任务数据 ──────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: dangerColumn.implicitHeight + 36
                radius: Theme.radius
                color: Theme.surface
                border.width: 1
                border.color: Theme.border

                ColumnLayout {
                    id: dangerColumn
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12

                    Text {
                        text: "任务数据"
                        color: Theme.text
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        StudioButton {
                            text: "清除已结束任务"
                            compact: true
                            enabled: (backend.doneCount + backend.failedCount) > 0
                            onClicked: backend.removeFinished()
                        }

                        StudioButton {
                            text: "清空全部任务"
                            compact: true
                            danger: true
                            enabled: backend.totalCount > 0
                            onClicked: backend.clearAll()
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "转换结果不会被删除，只清理列表记录。"
                        color: Theme.textFaint
                        font.pixelSize: 11
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }
        }
    }

    FolderDialog {
        id: folderDialog
        title: "选择输出目录"
        onAccepted: backend.chooseOutputDir(selectedFolder)
    }
}
