/**
 * @file CustomSpinBox.qml
 * @brief 通用数字输入框组件（也支持离散的枚举值）
 *
 * 提供统一样式的通用数字输入框组件，支持：
 * - 自定义颜色主题
 * - 悬停和按下效果
 * - 可配置的字体大小和圆角
 * - 数值范围控制
 * - 步长设置
 * - 与项目整体风格一致的外观
 *
 * @author Sakurakugu
 * @date 2025-09-05 01:04:35(UTC+8) 周五
 * @change 2025-09-05 18:17:08(UTC+8) 周五
 * @version 0.4.0
 */

import QtQuick
import QtQuick.Controls

SpinBox {
    id: root

    // 自定义外观属性
    property color textColor: ThemeManager.button2TextColor                  ///< 文本颜色
    property color disabledTextColor: ThemeManager.button2DisabledTextColor  ///< 禁用文本
    property color backgroundColor: ThemeManager.button2Color                ///< 背景颜色
    property color hoverColor: ThemeManager.button2HoverColor                ///< 悬停颜色
    property color pressedColor: ThemeManager.button2PressedColor            ///< 按下颜色
    property color disabledColor: ThemeManager.button2DisabledColor          ///< 禁用颜色
    property color borderColor: ThemeManager.borderColor                     ///< 边框颜色
    property color buttonColor: ThemeManager.button2Color                    ///< 按钮颜色
    property int fontSize: 14                                    ///< 字体大小
    property int radius: 4                                       ///< 圆角半径
    property int borderWidth: 1                                  ///< 边框宽度

    // 数值设置
    from: 0                                                      ///< 最小值
    to: 100                                                      ///< 最大值
    value: 0                                                     ///< 当前值
    stepSize: 1                                                  ///< 步长
    editable: true                                               ///< 是否允许手动输入

    // 尺寸设置
    implicitWidth: 150                                           ///< 默认宽度
    implicitHeight: 30                                           ///< 默认高度

    // 内容项（输入框）
    contentItem: TextInput {
        z: 2
        text: root.textFromValue(root.value, root.locale)
        font.pixelSize: root.fontSize
        color: root.enabled ? root.textColor : root.disabledTextColor
        selectionColor: root.textColor
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !root.editable
        validator: root.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        anchors.fill: parent
        anchors.leftMargin: 30
        anchors.rightMargin: 30

        onTextEdited: {
            if (root.editable) {
                root.value = root.valueFromText(text, root.locale);
            }
        }
    }

    // 背景
    background: Rectangle {
        implicitWidth: root.implicitWidth
        implicitHeight: root.implicitHeight
        color: {
            if (!root.enabled) {
                return root.disabledColor;
            }
            if (root.hovered) {
                return root.hoverColor;
            }
            return root.backgroundColor;
        }
        border.color: root.borderColor
        border.width: root.borderWidth
        radius: root.radius

        // 颜色过渡动画
        Behavior on color {
            ColorAnimation {
                duration: 150
            }
        }
    }

    // 减少按钮
    down.indicator: Rectangle {
        x: root.mirrored ? parent.width - width : 0
        height: parent.height
        implicitWidth: 30
        implicitHeight: root.implicitHeight
        color: {
            if (!root.enabled || root.value <= root.from) {
                return root.disabledColor;
            }
            if (root.down.pressed) {
                return root.pressedColor;
            }
            if (root.down.hovered) {
                return root.hoverColor;
            }
            return root.buttonColor;
        }
        border.color: root.borderColor
        border.width: root.borderWidth
        radius: root.radius

        Text {
            text: "-"
            font.pixelSize: root.fontSize + 2
            color: root.enabled && root.value > root.from ? root.textColor : root.disabledTextColor
            anchors.centerIn: parent
        }

        // 颜色过渡动画
        Behavior on color {
            ColorAnimation {
                duration: 150
            }
        }
    }

    // 增加按钮
    up.indicator: Rectangle {
        x: root.mirrored ? 0 : parent.width - width
        height: parent.height
        implicitWidth: 30
        implicitHeight: root.implicitHeight
        color: {
            if (!root.enabled || root.value >= root.to) {
                return root.disabledColor;
            }
            if (root.up.pressed) {
                return root.pressedColor;
            }
            if (root.up.hovered) {
                return root.hoverColor;
            }
            return root.buttonColor;
        }
        border.color: root.borderColor
        border.width: root.borderWidth
        radius: root.radius

        Text {
            text: "+"
            font.pixelSize: root.fontSize + 2
            color: root.enabled && root.value < root.to ? root.textColor : root.disabledTextColor
            anchors.centerIn: parent
        }

        // 颜色过渡动画
        Behavior on color {
            ColorAnimation {
                duration: 150
            }
        }
    }
}
