// ClickBlocker.qml
// 鼠标拦截器，用于防止点击穿透到下层
import QtQuick

Item {
    id: root
    anchors.fill: parent
    // z: 要自己设置

    // 是否启用拦截
    property bool enabled: true

    Rectangle {
        anchors.fill: parent
        color: "transparent"  // 默认透明
        visible: root.enabled

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            enabled: root.enabled // 只有当 enabled 为 true 时才拦截
            hoverEnabled: true // 鼠标悬停时也拦截
        }
    }
}
