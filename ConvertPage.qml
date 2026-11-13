import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

Item {
    id: page

    signal requestPage(string name)

    readonly property string kindLabel: backend.kindFilter === "audio" ? "音频"
                                        : backend.kindFilter === "image" ? "图片" : "视频"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 26
        spacing: 16

        // ── 标题 ──────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            ColumnLayout {
                spacing: 4

                Text {
                    text: "格式转换"
                    color: Theme.text
                    font.pixelSize: 26
                    font.weight: Font.Bold
                }

                Text {
                    color: Theme.textDim
                    font.pixelSize: 12
                    text: page.kindLabel + " · " + backend.tasks.visibleTotal + " 个任务"
                          + (backend.tasks.visibleRunning > 0
                             ? " · " + backend.tasks.visibleRunning + " 个转换中" : "")
                }
            }

            Item { Layout.fillWidth: true }

            StudioButton {
                text: "打开输出目录"
                icon: "folder"
                onClicked: backend.openOutputDir()
            }

            StudioButton {
                text: "添加文件"
                icon: "plus"
                primary: true
                onClicked: fileDialog.open()
            }
        }

        // ── ffmpeg 缺失提示 ───────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            visible: !backend.ffmpegReady
            implicitHeight: bannerLayout.implicitHeight + 26
            radius: 12
            color: Theme.warningSoft
            border.width: 1
            border.color: Theme.warning

            RowLayout {
                id: bannerLayout
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 10

                Icon {
                    Layout.alignment: Qt.AlignVCenter
                    name: "alert"
                    size: 16
                    color: Theme.warning
                }

                Text {
                    Layout.fillWidth: true
                    text: "未检测到转换组件，转换功能暂不可用。建议重新安装 Format Studio，或在设置中重新检测。"
                    color: Theme.text
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                StudioButton {
                    text: "去设置"
                    compact: true
                    onClicked: page.requestPage("settings")
                }
            }
        }

        // ── 拖放区 ────────────────────────────────────────────
        DropZone {
            Layout.fillWidth: true
            compact: backend.totalCount > 0
            onClicked: fileDialog.open()
            onFilesDropped: (urls) => backend.addFiles(urls)
        }

        // ── 输出设置 ──────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: optionsColumn.implicitHeight + 30
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.border

            ColumnLayout {
                id: optionsColumn
                anchors.fill: parent
                anchors.margins: 15
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Icon {
                        Layout.alignment: Qt.AlignVCenter
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

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.border
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 9

                    Text {
                        text: "格式"
                        color: Theme.textDim
                        font.pixelSize: 12
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Repeater {
                        model: {
                            var kind = backend.kindFilter === "audio" ? "audio"
                                     : backend.kindFilter === "image" ? "image" : "video"
                            return backend.formatsFor(kind)
                        }

                        delegate: FormatChip {
                            label: modelData.label
                            note: modelData.note
                            selected: backend.currentFormat === modelData.ext
                            onClicked: backend.currentFormat = modelData.ext
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: "批量应用 · " + page.kindLabel
                        color: Theme.textFaint
                        font.pixelSize: 11
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Text {
                        text: "画质"
                        color: Theme.textDim
                        font.pixelSize: 12
                    }

                    Segmented {
                        id: qualitySegment
                        options: ["高画质", "均衡", "小体积"]
                        currentIndex: backend.quality
                        onActivated: (index) => backend.quality = index
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: "同时转换 " + backend.concurrency + " 个任务"
                        color: Theme.textFaint
                        font.pixelSize: 11
                    }
                }
            }
        }

        // ── 队列标题 ──────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: "任务队列"
                color: Theme.text
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }

            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                height: 20
                width: queueCount.implicitWidth + 16
                radius: 10
                color: Theme.surfaceSunken
                border.width: 1
                border.color: Theme.border

                Text {
                    id: queueCount
                    anchors.centerIn: parent
                    text: String(list.count)
                    color: Theme.textDim
                    font.pixelSize: 10
                }
            }

            Item { Layout.fillWidth: true }

            StudioButton {
                text: "清除已结束"
                compact: true
                visible: (backend.tasks.visibleDone + backend.tasks.visibleFailed) > 0
                onClicked: backend.removeFinishedKind(backend.kindFilter)
            }

            StudioButton {
                text: "清空"
                compact: true
                danger: true
                visible: backend.tasks.visibleTotal > 0
                onClicked: backend.clearKind(backend.kindFilter)
            }
        }

        // ── 任务列表 ──────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                anchors.fill: parent
                model: backend.tasks
                spacing: 10
                topMargin: 2
                bottomMargin: 8
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    id: verticalBar
                    policy: ScrollBar.AsNeeded
                    contentItem: Rectangle {
                        implicitWidth: 6
                        radius: 3
                        color: verticalBar.pressed ? Theme.textFaint
                               : verticalBar.active ? Theme.borderStrong : Theme.border
                        opacity: verticalBar.active ? 1 : 0.55
                    }
                    background: Item {}
                }

                delegate: TaskCard {
                    width: list.width - 12

                    taskId: model.id
                    fileName: model.fileName
                    kind: model.kind
                    targetFormat: model.format
                    status: model.status
                    progress: model.progress
                    speed: model.speed
                    eta: model.eta
                    errorText: model.errorText
                    thumbnail: model.thumbnail
                    detail: model.detail
                    durationMs: model.durationMs
                    inputSize: model.inputSize
                    outputSize: model.outputSize
                    statusText: model.statusText
                }

                add: Transition {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 180 }
                }
            }

            EmptyState {
                anchors.centerIn: parent
                visible: list.count === 0
                icon: "layers"
                title: "还没有任务"
                subtitle: "把视频、音频或图片拖到上方，或点击「添加文件」"
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 26
                visible: list.contentHeight > list.height
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(Theme.bg.r, Theme.bg.g, Theme.bg.b, 0.0) }
                    GradientStop { position: 1.0; color: Theme.bg }
                }
            }
        }

        // ── 底部操作栏 ────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 66
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 12
                spacing: 14

                Text {
                    text: "已完成 " + backend.tasks.visibleDone + " / " + backend.tasks.visibleTotal
                    color: Theme.text
                    font.pixelSize: 12
                }

                Item {
                    Layout.preferredWidth: 160
                    Layout.preferredHeight: 6

                    Rectangle {
                        id: overallTrack
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: 6
                        radius: 3
                        color: Theme.surfaceSunken
                    }

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.max(0, Math.min(100, backend.tasks.visibleProgress)) / 100 * overallTrack.width
                        height: 6
                        radius: 3
                        color: Theme.accent
                        Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                    }
                }

                Text {
                    text: backend.tasks.visibleProgress + "%"
                    color: Theme.textDim
                    font.pixelSize: 11
                    Layout.preferredWidth: 34
                }

                Item { Layout.fillWidth: true }

                StudioButton {
                    text: "停止"
                    icon: "stop"
                    danger: true
                    visible: backend.runningCount > 0 || backend.queueActive
                    onClicked: backend.stopAll()
                }

                StudioButton {
                    text: backend.queueActive ? "暂停队列" : "开始转换"
                    icon: backend.queueActive ? "pause" : "play"
                    primary: !backend.queueActive
                    enabled: backend.queueActive ? true : backend.tasks.visibleQueued > 0
                    onClicked: backend.toggleQueue()
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "选择要转换的文件"
        fileMode: FileDialog.OpenFiles
        nameFilters: [backend.nameFilter(backend.kindFilter)]
        onAccepted: backend.addFiles(selectedFiles)
    }

    FolderDialog {
        id: folderDialog
        title: "选择输出目录"
        onAccepted: backend.chooseOutputDir(selectedFolder)
    }
}
