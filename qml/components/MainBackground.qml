/**
 * @brief 主背景组件
 *
 * 该组件用于创建应用的主背景，通常是一个矩形区域，用于包裹应用的内容。
 *
 * @author Sakurakugu
 * @date 2025-09-04 23:39:30(UTC+8) 周四
 * @change 2025-09-04 23:39:30(UTC+8) 周四
 */

import QtQuick

Rectangle {
    anchors.fill: parent
    anchors.margins: 0
    color: ThemeManager.backgroundColor
    radius: 10  // 添加圆角
    border.color: ThemeManager.borderColor
    border.width: 1
    clip: true
    z: -10                           ///< 确保背景在所有内容下方
}
