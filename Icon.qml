import QtQuick

Item {
    id: root

    property string name: ""
    property color color: Theme.text
    property real size: 20

    implicitWidth: size
    implicitHeight: size

    readonly property string hex: {
        function part(v) {
            var n = Math.max(0, Math.min(255, Math.round(v * 255)))
            var s = n.toString(16)
            return s.length === 1 ? "0" + s : s
        }
        return part(root.color.r) + part(root.color.g) + part(root.color.b)
    }

    Image {
        anchors.fill: parent
        visible: root.name !== ""
        source: root.name !== "" ? "image://icons/" + root.name + "/" + root.hex : ""
        sourceSize.width: Math.round(root.size * 2)
        sourceSize.height: Math.round(root.size * 2)
        fillMode: Image.PreserveAspectFit
        smooth: true
        asynchronous: true
    }
}
