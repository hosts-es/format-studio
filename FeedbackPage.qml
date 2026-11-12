import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: page

    signal requestPage(string name)

    property int feedbackType: 0
    readonly property int maxLength: 2000

    ColumnLayout {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 26
        anchors.bottomMargin: 26
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 52, 880)
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            IconButton {
                icon: "chevron-left"
                tooltip: "返回"
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 8
                onClicked: page.requestPage("about")
            }

            ColumnLayout {
                spacing: 3

                Text {
                    text: "提交反馈"
                    color: Theme.text
                    font.pixelSize: 24
                    font.weight: Font.Bold
                }

                Text {
                    text: "选择类型并写下你的问题或期待"
                    color: Theme.textDim
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }
        }

        Text {
            Layout.topMargin: 4
            text: "反馈类型"
            color: Theme.textDim
            font.pixelSize: 12
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                radius: 14
                color: page.feedbackType === 0 ? Theme.accentSoft : Theme.surface
                border.width: page.feedbackType === 0 ? 1.4 : 1
                border.color: page.feedbackType === 0 ? Theme.accent : Theme.border
                Behavior on color { ColorAnimation { duration: 120 } }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Rectangle {
                        width: 36
                        height: 36
                        radius: 10
                        color: page.feedbackType === 0 ? Theme.accent : Theme.surfaceSunken

                        Icon {
                            anchors.centerIn: parent
                            name: "alert"
                            size: 18
                            color: page.feedbackType === 0 ? Theme.accentText : Theme.textDim
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: "修 Bug 的反馈"
                            color: Theme.text
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: "崩溃、转换失败、界面异常"
                            color: Theme.textDim
                            font.pixelSize: 11
                        }
                    }

                    Icon {
                        name: "check"
                        size: 16
                        color: Theme.accent
                        visible: page.feedbackType === 0
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: page.feedbackType = 0
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                radius: 14
                color: page.feedbackType === 1 ? Theme.accentSoft : Theme.surface
                border.width: page.feedbackType === 1 ? 1.4 : 1
                border.color: page.feedbackType === 1 ? Theme.accent : Theme.border
                Behavior on color { ColorAnimation { duration: 120 } }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Rectangle {
                        width: 36
                        height: 36
                        radius: 10
                        color: page.feedbackType === 1 ? Theme.accent : Theme.surfaceSunken

                        Icon {
                            anchors.centerIn: parent
                            name: "sparkles"
                            size: 18
                            color: page.feedbackType === 1 ? Theme.accentText : Theme.textDim
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: "功能期待的反馈"
                            color: Theme.text
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: "想增加的格式、参数或流程"
                            color: Theme.textDim
                            font.pixelSize: 11
                        }
                    }

                    Icon {
                        name: "check"
                        size: 16
                        color: Theme.accent
                        visible: page.feedbackType === 1
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: page.feedbackType = 1
                }
            }
        }

        Text {
            Layout.topMargin: 4
            text: "留言"
            color: Theme.textDim
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 120
            Layout.maximumHeight: 360
            radius: 14
            color: Theme.surface
            border.width: 1
            border.color: messageArea.activeFocus ? Theme.accent : Theme.border

            Rectangle {
                anchors.fill: parent
                anchors.margins: 14
                color: "transparent"

                Text {
                    anchors.fill: parent
                    visible: messageArea.length === 0
                    text: "描述一下遇到的问题，或你希望增加什么功能…"
                    color: Theme.textFaint
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
            }

            TextArea {
                id: messageArea
                anchors.fill: parent
                anchors.margins: 14
                padding: 0
                color: Theme.text
                font.pixelSize: 12
                wrapMode: TextArea.Wrap
                selectByMouse: true
                background: null

                onTextChanged: {
                    if (length > page.maxLength)
                        remove(page.maxLength, length - page.maxLength)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: tipsColumn.implicitHeight + 32
            radius: 14
            color: Theme.surface
            border.width: 1
            border.color: Theme.border

            ColumnLayout {
                id: tipsColumn
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                Text {
                    text: "写好反馈的小提示"
                    color: Theme.text
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                Repeater {
                    model: [
                        "复现步骤：做了什么、点了哪里，越具体越好",
                        "文件信息：格式、大小、来源（本地录制 / 下载 / 手机）",
                        "期望结果：你希望它表现成什么样"
                    ]

                    delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            width: 4
                            height: 4
                            radius: 2
                            color: Theme.textFaint
                        }

                        Text {
                            Layout.fillWidth: true
                            text: modelData
                            color: Theme.textDim
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: "提交后会打开 " + updates.repo + " 的 GitHub Issue 页面，标题与内容已自动填好，确认后即发布。"
                color: Theme.textFaint
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }

            Text {
                text: messageArea.length + " / " + page.maxLength
                color: Theme.textFaint
                font.pixelSize: 11
            }

            StudioButton {
                text: "提交"
                icon: "check"
                primary: true
                enabled: messageArea.text.trim().length > 0
                onClicked: updates.submitFeedback(page.feedbackType, messageArea.text)
            }
        }
    }
}
