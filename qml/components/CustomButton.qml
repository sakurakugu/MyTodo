/**
 * @file CustomButton.qml
 * @brief 通用按钮组件
 *
 * 提供统一样式的通用按钮组件，支持：
 * - 自定义颜色主题
 * - 悬停和按下效果
 * - 可配置的字体大小和圆角
 * - 扁平化和填充设计风格
 *
 * @author Sakurakugu
 * @date 2025-08-17 07:17:29(UTC+8) 周日
 * @change 2025-09-04 23:39:30(UTC+8) 周四
 * @version 0.4.0
 */

import QtQuick
import QtQuick.Controls

Button {
    id: root

    // 自定义外观属性
    property color textColor: !is2ndColor ? ThemeManager.buttonTextColor : ThemeManager.button2TextColor                          ///< 文本颜色
    property color disabledTextColor: !is2ndColor ? ThemeManager.buttonDisabledTextColor : ThemeManager.button2DisabledTextColor  ///< 禁用文本
    property color backgroundColor: !is2ndColor ? ThemeManager.buttonColor : ThemeManager.button2Color                            ///< 背景颜色
    property color hoverColor: !is2ndColor ? ThemeManager.buttonHoverColor : ThemeManager.button2HoverColor                       ///< 悬停颜色
    property color pressedColor: !is2ndColor ? ThemeManager.buttonPressedColor : ThemeManager.button2PressedColor                 ///< 按下颜色
    property color disabledColor: !is2ndColor ? ThemeManager.buttonDisabledColor : ThemeManager.button2DisabledColor              ///< 禁用颜色
    property color borderColor: ThemeManager.borderColor         ///< 边框颜色
    property bool is2ndColor: false                              ///< 是否是副按钮颜色
    property int fontSize: 14                                    ///< 字体大小
    property int radius: 4                                       ///< 圆角半径
    property bool isFlat: false                                  ///< 是否为扁平化样式
    property int borderWidth: 1                                  ///< 边框宽度

    // 尺寸设置
    implicitWidth: contentItem.implicitWidth + 40      ///< 默认宽度
    implicitHeight: contentItem.implicitHeight + 25    ///< 默认高度

    // 背景设置
    background: Rectangle {
        color: {
            if (!root.enabled) {
                return root.disabledColor;
            }
            if (root.pressed) {
                return root.pressedColor;
            }
            if (root.hovered) {
                return root.hoverColor;
            }
            return root.isFlat ? "transparent" : root.backgroundColor;
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

    // 内容设置
    contentItem: Text {
        text: root.text
        font.pixelSize: root.fontSize
        color: root.enabled ? root.textColor : root.disabledTextColor
        horizontalAlignment: Text.AlignHCenter // 水平居中
        verticalAlignment: Text.AlignVCenter // 垂直居中
    }
}
