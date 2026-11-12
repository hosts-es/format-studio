import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: root

    property string taskId: ""
    property string fileName: ""
    property string kind: "video"
    property string targetFormat: "mp4"
    property int status: 0
    property real progress: 0
    property string speed: ""
    property real eta: -1
    property string errorText: ""
    property string thumbnail: ""
    property string detail: ""
    property real durationMs: 0
    property real inputSize: 0
    property real outputSize: 0
    property string statusText: "等待中"

    readonly property bool isQueued: status === 0
    readonly property bool isRunning: status === 1
    readonly property bool isDone: status === 2
    readonly property bool isFailed: status === 3
    readonly property bool isCanceled: status === 4
    readonly property bool isSkipped: status === 5

    readonly property var formatOptions: backend.formatsFor(root.kind)
    readonly property var formatLabels: formatOptions.map(function (f) { return f.label })

    height: (root.isFailed && root.errorText !== "") ? 116 : 92
    radius: 14
    color: hover.hovered ? Theme.surfaceHi : Theme.surface
    border.width: 1
    border.color: root.isRunning ? Theme.accent : (root.isFailed ? Theme.danger : Theme.border)
    Behavior on color { ColorAnimation { duration: 120 } }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 13
        spacing: 14

        // ── 缩略图 ────────────────────────────────────────────
        Rectangle {
            Layout.preferredWidth: 106
            Layout.preferredHeight: 64
            Layout.alignment: Qt.AlignVCenter
            radius: 9
            clip: true
            color: Theme.surfaceSunken

            Image {
                anchors.fill: parent
                source: root.thumbnail
                visible: root.thumbnail !== ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                smooth: true
            }

            Icon {
                anchors.centerIn: parent
                visible: root.thumbnail === ""
                name: root.kind === "audio" ? "music" : root.kind === "image" ? "image" : "video"
                size: 24
                color: Theme.textFaint
            }

            Rectangle {
                anchors.fill: parent
                color: Theme.bg
                opacity: root.isRunning ? 0.35 : 0
                Behavior on opacity { NumberAnimation { duration: 220 } }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: 5
                height: 16
                width: durationLabel.implicitWidth + 10
                radius: 5
                color: "#A6000000"
                visible: root.durationMs > 0

                Text {
                    id: durationLabel
                    anchors.centerIn: parent
                    text: Theme.formatClock(root.durationMs)
                    color: "#FFFFFF"
                    font.pixelSize: 9
                }
            }
        }

        // ── 信息 ──────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    text: root.fileName
                    color: Theme.text
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    elide: Text.ElideMiddle
                    maximumLineCount: 1
                }

                Rectangle {
                    Layout.preferredHeight: 20
                    Layout.preferredWidth: statusLabel.implicitWidth + 16
                    radius: 6
                    color: root.isDone ? Theme.successSoft
                           : root.isFailed ? Theme.dangerSoft
                           : root.isRunning ? Theme.accentSoft : Theme.surfaceSunken

                    Text {
                        id: statusLabel
                        anchors.centerIn: parent
                        text: root.statusText
                        font.pixelSize: 10
                        color: root.isDone ? Theme.success
                               : root.isFailed ? Theme.danger
                               : root.isRunning ? Theme.accent : Theme.textDim
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    Layout.fillWidth: true
                    text: root.detail
                    color: Theme.textDim
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Text {
                    text: "→"
                    visible: root.isRunning || root.isDone
                    color: Theme.textFaint
                    font.pixelSize: 11
                }

                Text {
                    text: root.targetFormat.toUpperCase()
                    visible: root.isRunning || root.isDone
                    color: Theme.accent
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 16

                    Rectangle {
                        id: track
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: 5
                        radius: 3
                        color: Theme.surfaceSunken
                    }

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.max(0, Math.min(1, root.progress / 100)) * track.width
                        height: 5
                        radius: 3
                        color: root.isFailed ? Theme.danger : root.isDone ? Theme.success : Theme.accent
                        Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                    }
                }

                Text {
                    font.pixelSize: 11
                    color: Theme.textFaint
                    visible: text !== ""
                    text: {
                        if (root.isRunning)
                            return (root.speed !== "" ? root.speed + " · " : "") + Theme.formatEta(root.eta)
                        if (root.isDone)
                            return Theme.formatSize(root.outputSize)
                        if (root.isQueued)
                            return Theme.formatSize(root.inputSize)
                        return ""
                    }
                }

                Text {
                    Layout.preferredWidth: 34
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: 11
                    color: root.isDone ? Theme.success : root.isFailed ? Theme.danger : Theme.textDim
                    text: (root.isRunning || root.isDone) ? Math.round(root.progress) + "%" : ""
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.errorText !== ""
                text: root.errorText
                color: Theme.danger
                font.pixelSize: 11
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }

        // ── 操作 ──────────────────────────────────────────────
        RowLayout {
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            ComboBox {
                id: formatCombo

                Layout.preferredWidth: 92
                Layout.preferredHeight: 30
                Layout.alignment: Qt.AlignVCenter
                visible: !root.isRunning && !root.isDone
                model: root.formatLabels
                font.pixelSize: 11
                padding: 0

                function syncIndex() {
                    var options = root.formatOptions
                    for (var i = 0; i < options.length; ++i) {
                        if (options[i].ext === root.targetFormat) {
                            currentIndex = i
                            return
                        }
                    }
                }

                Component.onCompleted: syncIndex()

                Connections {
                    target: root
                    function onTargetFormatChanged() { formatCombo.syncIndex() }
                }

                onActivated: {
                    var options = root.formatOptions
                    if (index >= 0 && index < options.length)
                        backend.setTaskFormat(root.taskId, options[index].ext)
                }

                contentItem: Text {
                    leftPadding: 9
                    text: formatCombo.displayText
                    color: Theme.text
                    font.pixelSize: 11
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                indicator: Icon {
                    x: formatCombo.width - 20
                    y: (formatCombo.height - 12) / 2
                    name: "chevron-down"
                    size: 12
                    color: Theme.textDim
                }

                background: Rectangle {
                    radius: 8
                    color: Theme.surfaceSunken
                    border.width: 1
                    border.color: formatCombo.hovered ? Theme.borderStrong : Theme.border
                }

                delegate: ItemDelegate {
                    id: formatDelegate
                    width: formatCombo.width
                    height: 30
                    highlighted: formatCombo.highlightedIndex === index

                    contentItem: Text {
                        leftPadding: 9
                        text: modelData
                        color: Theme.text
                        font.pixelSize: 11
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        radius: 7
                        color: formatDelegate.highlighted ? Theme.accentSoft : "transparent"
                    }
                }

                popup: Popup {
                    y: formatCombo.height + 4
                    width: formatCombo.width
                    implicitHeight: Math.min(300, formatList.contentHeight + 10)
                    padding: 5

                    background: Rectangle {
                        color: Theme.surfaceHi
                        radius: 10
                        border.width: 1
                        border.color: Theme.border
                    }

                    contentItem: ListView {
                        id: formatList
                        clip: true
                        implicitHeight: contentHeight
                        model: formatCombo.delegateModel
                        currentIndex: formatCombo.highlightedIndex
                        boundsBehavior: Flickable.StopAtBounds
                    }
                }
            }

            IconButton {
                visible: root.isQueued
                icon: "play"
                tooltip: "立即转换"
                onClicked: backend.startTask(root.taskId)
            }

            IconButton {
                visible: root.isRunning
                icon: "stop"
                tooltip: "取消该任务"
                iconColor: Theme.danger
                onClicked: backend.cancelTask(root.taskId)
            }

            IconButton {
                visible: root.isDone
                icon: "folder"
                tooltip: "打开输出位置"
                onClicked: backend.revealOutput(root.taskId)
            }

            IconButton {
                visible: root.isFailed || root.isCanceled || root.isSkipped
                icon: "refresh"
                tooltip: "重新转换"
                onClicked: backend.retryTask(root.taskId)
            }

            IconButton {
                visible: !root.isRunning
                icon: "trash"
                tooltip: "移除任务"
                onClicked: backend.removeTask(root.taskId)
            }
        }
    }

    HoverHandler { id: hover }
}
