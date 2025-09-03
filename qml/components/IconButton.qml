/**
 * @file iconButton.qml
 * @brief 自定义按钮组件
 *
 * 提供统一样式的按钮组件，支持：
 * - 自定义颜色主题
 * - 悬停和按下效果
 * - 可配置的字体大小
 * - 扁平化设计风格
 *
 * @author Sakurakugu
 * @date 2025-08-24 07:17:29(UTC+8) 周日
 * @version 2025-08-21 21:31:41(UTC+8) 周四
 */

import QtQuick

/**
 * @brief 自定义按钮组件
 *
 * 基于MouseArea实现的自定义按钮，提供统一的视觉风格和交互效果。
 * 支持主题色彩定制和多种视觉状态。
 */
Item {
    id: iconButton

    // 自定义外观属性
    property color textColor: ThemeManager.textColor  ///< 文本颜色，支持主题
    property color backgroundColor: "transparent" ///< 背景颜色
    property int fontSize: 14                     ///< 字体大小
    property bool isFlat: true                    ///< 是否为扁平化样式
    property string text: ""                      ///< 按钮文本

    // 状态属性
    property bool isHovered: mouseArea.containsMouse   ///< 悬停状态
    property bool isPressed: mouseArea.pressed         ///< 按下状态
    property bool isFocused: false                     ///< 焦点状态

    // 信号
    signal clicked                              ///< 点击信号

    implicitWidth: 30                           ///< 默认宽度
    implicitHeight: 30                          ///< 默认高度

    /**
     * @brief 按钮背景样式
     *
     * 自定义背景矩形，支持悬停和按下状态的视觉反馈。
     */
    Rectangle {
        anchors.fill: parent
        color: iconButton.backgroundColor           ///< 背景色
        radius: 3                                   ///< 圆角半径
        border.width: iconButton.isHovered ? 1 : 0  ///< 悬停时显示边框
        border.color: iconButton.textColor          ///< 边框颜色与文本颜色一致
        opacity: iconButton.isPressed ? 0.7 : (iconButton.isHovered ? 0.9 : 1.0) ///< 状态透明度

        /// 透明度变化动画
        Behavior on opacity {
            NumberAnimation {
                duration: 100
            }
        }
    }

    // 按钮文本内容
    Text {
        anchors.centerIn: parent
        text: iconButton.text                       ///< 显示按钮文本
        color: iconButton.textColor                 ///< 文本颜色
        font.pixelSize: iconButton.fontSize         ///< 字体大小
        font.family: "iconfont"                     ///< 使用图标字体
        horizontalAlignment: Text.AlignHCenter      ///< 水平居中
        verticalAlignment: Text.AlignVCenter        ///< 垂直居中
        font.bold: iconButton.isFocused             ///< 焦点时加粗
    }

    // 鼠标交互区域
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: iconButton.clicked()
    }
}
