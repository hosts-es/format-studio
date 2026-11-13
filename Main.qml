import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ApplicationWindow {
    id: window

    width: 1240
    height: 800
    minimumWidth: 1000
    minimumHeight: 660
    visible: true
    title: "Format Studio"
    color: Theme.bg

    property int currentPage: 0

    function selectPageByName(name) {
        switch (name) {
        case "settings": currentPage = 1; break
        case "about": currentPage = 2; break
        case "docs": currentPage = 3; break
        case "feedback": currentPage = 4; break
        default: currentPage = 0; break
        }
    }

    function selectCategory(kind) {
        backend.kindFilter = kind
        selectPageByName("convert")
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── 侧边栏 ────────────────────────────────────────────
        Rectangle {
            Layout.preferredWidth: Theme.sidebarWidth
            Layout.fillHeight: true
            color: Theme.sidebar

            Rectangle {
                width: 1
                height: parent.height
                anchors.right: parent.right
                color: Theme.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 9
                spacing: 4

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 58

                    Image {
                        id: appLogo
                        width: 46
                        height: 46
                        anchors.centerIn: parent
                        source: "qrc:/icons/app.png"
                        sourceSize.width: 92
                        sourceSize.height: 92
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }

                    Rectangle {
                        anchors.fill: appLogo
                        radius: 10
                        color: "transparent"
                        border.width: 1
                        border.color: Theme.dark ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.08)
                    }
                }

                NavButton {
                    label: "视频"
                    icon: "video"
                    selected: window.currentPage === 0 && backend.kindFilter === "video"
                    badge: backend.countKind("video")
                    onClicked: window.selectCategory("video")
                }

                NavButton {
                    label: "音频"
                    icon: "music"
                    selected: window.currentPage === 0 && backend.kindFilter === "audio"
                    badge: backend.countKind("audio")
                    onClicked: window.selectCategory("audio")
                }

                NavButton {
                    label: "图片"
                    icon: "image"
                    selected: window.currentPage === 0 && backend.kindFilter === "image"
                    badge: backend.countKind("image")
                    onClicked: window.selectCategory("image")
                }

                NavButton {
                    label: "文档"
                    icon: "doc"
                    soon: true
                    selected: window.currentPage === 3
                    onClicked: window.selectPageByName("docs")
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 1
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    color: Theme.border
                }

                ThemeSwitch {
                    onClicked: backend.themeMode = Theme.dark ? "light" : "dark"
                }

                NavButton {
                    label: "设置"
                    icon: "settings"
                    selected: window.currentPage === 1
                    onClicked: window.selectPageByName("settings")
                }

                NavButton {
                    label: "关于"
                    icon: "info"
                    selected: window.currentPage === 2
                    onClicked: window.selectPageByName("about")
                }

                Item { Layout.preferredHeight: 6 }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: "V" + updates.currentVersion
                    color: Theme.textFaint
                    font.pixelSize: 9
                }

                Item { Layout.preferredHeight: 4 }
            }
        }

        // ── 主内容 ────────────────────────────────────────────
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.currentPage

            ConvertPage {
                onRequestPage: (name) => window.selectPageByName(name)
            }

            SettingsPage {}
            AboutPage {
                onRequestPage: (name) => window.selectPageByName(name)
            }
            DocsPage {}
            FeedbackPage {
                onRequestPage: (name) => window.selectPageByName(name)
            }
        }
    }

    // ── 新版本提示 ────────────────────────────────────────────
    Item {
        id: updateOverlay
        anchors.fill: parent
        visible: false
        z: 90

        Rectangle {
            anchors.fill: parent
            color: Theme.dark ? "#B3000000" : "#66000000"

            MouseArea {
                anchors.fill: parent
                onClicked: updateOverlay.visible = false
            }
        }

        Rectangle {
            width: Math.min(520, window.width - 120)
            implicitHeight: updateDialogColumn.implicitHeight + 56
            anchors.centerIn: parent
            radius: 18
            color: Theme.surface
            border.width: 1
            border.color: Theme.border

            ColumnLayout {
                id: updateDialogColumn
                anchors.fill: parent
                anchors.margins: 28
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Icon {
                        name: "download"
                        size: 18
                        color: Theme.accent
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "发现新版本 V" + updates.latestVersion
                        color: Theme.text
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: "当前版本 V" + updates.currentVersion + "，是否立即更新？"
                    color: Theme.textDim
                    font.pixelSize: 12
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(200, updateNotes.implicitHeight + 24)
                    visible: updates.releaseNotes !== ""
                    radius: 10
                    color: Theme.surfaceSunken
                    border.width: 1
                    border.color: Theme.border
                    clip: true

                    Flickable {
                        anchors.fill: parent
                        anchors.margins: 12
                        contentWidth: width
                        contentHeight: updateNotes.implicitHeight
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds

                        Text {
                            id: updateNotes
                            width: parent.width
                            text: updates.releaseNotes
                            color: Theme.textDim
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Item { Layout.fillWidth: true }

                    StudioButton {
                        text: "稍后"
                        onClicked: updateOverlay.visible = false
                    }

                    StudioButton {
                        text: "立即更新"
                        icon: "download"
                        primary: true
                        onClicked: {
                            updateOverlay.visible = false
                            updates.downloadAndInstall()
                        }
                    }
                }
            }
        }
    }

    Toast {
        id: toast
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 96
        anchors.rightMargin: 22
        z: 100
    }

    Connections {
        target: backend
        function onToast(text, level) { toast.show(text, level) }
    }

    Connections {
        target: updates
        function onToast(text, level) { toast.show(text, level) }
        function onReleaseInfoChanged() {
            if (updates.updateAvailable)
                updateOverlay.visible = true
        }
    }

    DropArea {
        anchors.fill: parent
        onDropped: (drag) => backend.addFiles(drag.urls)
    }
}
