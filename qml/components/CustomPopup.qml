/**
 * @brief 自定义弹窗组件
 *
 * 该组件用于创建自定义的弹窗，用于显示提示信息、确认操作等。
 *
 * @author Sakurakugu
 * @date 2025-09-06 08:22:34(UTC+8) 周六
 * @change 2025-09-06 16:55:02(UTC+8) 周六
 * @version 0.4.0
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Popup {
    modal: false // 非模态，允许同时打开该弹窗时与其他地方交互
    focus: true
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        color: ThemeManager.secondaryBackgroundColor
        border.color: ThemeManager.borderColor
        radius: 5

        // 颜色过渡动画
        Behavior on color {
            ColorAnimation {
                duration: ThemeManager.colorAnimationDuration
            }
        }

        Behavior on border.color {
            ColorAnimation {
                duration: ThemeManager.colorAnimationDuration
            }
        }
    }

    contentItem: Rectangle {
        color: ThemeManager.backgroundColor
        border.color: ThemeManager.borderColor
        border.width: 1
        radius: 5

        // 颜色过渡动画
        Behavior on color {
            ColorAnimation {
                duration: ThemeManager.colorAnimationDuration
            }
        }

        Behavior on border.color {
            ColorAnimation {
                duration: ThemeManager.colorAnimationDuration
            }
        }
    }
}
