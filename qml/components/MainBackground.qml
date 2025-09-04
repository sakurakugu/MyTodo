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
