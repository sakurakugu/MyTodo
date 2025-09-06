/**
 * @brief 鼠标拦截器，用于防止点击穿透到下层
 *
 * 该组件用于在需要防止点击穿透的场景下，拦截鼠标点击事件，防止点击事件传递到下层元素。
 *
 * @author Sakurakugu
 * @date 2025-09-02 01:16:59(UTC+8) 周二
 * @change 2025-09-06 00:46:10(UTC+8) 周六
 * @version 0.4.0
 */
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
