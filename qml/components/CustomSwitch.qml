/**
 * @file CustomSwitch.qml
 * @brief 自定义开关组件
 *
 * 该文件定义了一个自定义的开关组件，支持主题切换和动画效果。
 *
 * @author Sakurakugu
 * @date 2025-09-04 14:37:56(UTC+8) 周四
 * @change 2025-09-10 01:01:04(UTC+8) 周三
 */
import QtQuick
import QtQuick.Effects

Item {
    id: root

    // 公开属性
    property bool checked: false
    property bool enabled: true
    property string text: ""
    property int textSpacing: 8
    property bool isSwitchOnLeft: true // 开关在左还是右边

    // 信号
    signal toggled(bool checked)

    // 尺寸设置
    implicitWidth: switchTrack.width + label.implicitWidth + textSpacing + 4
    implicitHeight: Math.max(switchTrack.height, label.implicitHeight)

    // 文本标签
    Text {
        text: root.text
        anchors.left: switchTrack.right
        anchors.leftMargin: root.textSpacing
        anchors.verticalCenter: parent.verticalCenter
        color: root.enabled ? ThemeManager.textColor : ThemeManager.secondaryTextColor
        font.pixelSize: 14
        visible: !root.isSwitchOnLeft

        // 文本颜色过渡动画
        Behavior on color {
            ColorAnimation {
                duration: ThemeManager.colorAnimationDuration
            }
        }
    }

    // 开关轨道
    Rectangle {
        id: switchTrack
        width: 36
        height: 20
        radius: height / 2
        anchors.verticalCenter: parent.verticalCenter
        color: {
            if (!root.enabled) {
                return ThemeManager.buttonDisabledColor;
            }
            return root.checked ? ThemeManager.successColor : ThemeManager.button2Color;
        }

        // 过渡动画
        Behavior on color {
            ColorAnimation {
                duration: ThemeManager.colorAnimationDuration
            }
        }

        // 内部阴影效果
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: parent.radius - 1
            color: "transparent"
            border.width: 1
            border.color: root.checked ? Qt.darker(ThemeManager.successColor, 1.1) : Qt.darker(ThemeManager.button2Color, 1.1)
            opacity: 0.3

            // 边框颜色过渡动画
            Behavior on border.color {
                ColorAnimation {
                    duration: ThemeManager.colorAnimationDuration
                }
            }
        }
    }

    // 开关手柄
    Rectangle {
        id: handle
        width: 14
        height: 14
        radius: width / 2
        color: root.enabled ? "white" : ThemeManager.button2DisabledTextColor
        anchors.verticalCenter: switchTrack.verticalCenter
        x: switchTrack.x + (root.checked ? switchTrack.width - width - 3 : 3)

        // 颜色过渡动画
        Behavior on color {
            ColorAnimation {
                duration: ThemeManager.colorAnimationDuration
            }
        }

        // 位置动画
        Behavior on x {
            NumberAnimation {
                duration: ThemeManager.colorAnimationDuration
                easing.type: Easing.InOutQuad
            }
        }

        // 手柄阴影
        layer.enabled: root.enabled
        layer.effect: MultiEffect {
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 1
            shadowBlur: 2.0
            shadowColor: ThemeManager.shadowColor
        }
    }

    // 文本标签
    Text {
        id: label
        text: root.text
        anchors.left: switchTrack.right
        anchors.leftMargin: root.textSpacing
        anchors.verticalCenter: parent.verticalCenter
        color: root.enabled ? ThemeManager.textColor : ThemeManager.secondaryTextColor
        font.pixelSize: 14
        visible: root.isSwitchOnLeft

        // 文本颜色过渡动画
        Behavior on color {
            ColorAnimation {
                duration: ThemeManager.colorAnimationDuration
            }
        }
    }

    // 鼠标区域
    MouseArea {
        anchors.fill: parent
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            root.checked = !root.checked;
            root.toggled(root.checked);
        }
    }

    // 提供类似于标准Switch的方法
    function toggle() {
        if (root.enabled) {
            checked = !checked;
            toggled(checked);
        }
    }
}
