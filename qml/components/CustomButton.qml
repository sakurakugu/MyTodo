/**
 * @file CustomButton.qml
 * @brief 自定义按钮组件
 * 
 * 提供统一样式的按钮组件，支持：
 * - 自定义颜色主题
 * - 悬停和按下效果
 * - 可配置的字体大小
 * - 扁平化设计风格
 * 
 * @author MyTodo Team
 * @date 2024
 */

import QtQuick
import QtQuick.Controls

/**
 * @brief 自定义按钮组件
 * 
 * 基于Qt Button扩展的自定义按钮，提供统一的视觉风格和交互效果。
 * 支持主题色彩定制和多种视觉状态。
 */
Button {
    id: customButton
    
    // 主题支持
    property bool isDarkMode: false             ///< 深色模式开关
    
    // 自定义外观属性
    property color textColor: isDarkMode ? "#ecf0f1" : "black"  ///< 文本颜色，支持主题
    property color backgroundColor: "transparent" ///< 背景颜色
    property int fontSize: 14                   ///< 字体大小
    property bool isFlat: true                  ///< 是否为扁平化样式
    
    flat: isFlat                ///< 应用扁平化样式
    implicitWidth: Math.max(80, contentItem.implicitWidth + 20)  ///< 自适应宽度，最小80px，内容宽度+20px边距
    implicitHeight: 40          ///< 默认高度
    
    /**
     * @brief 按钮背景样式
     * 
     * 自定义背景矩形，支持悬停和按下状态的视觉反馈。
     */
    background: Rectangle {
        color: backgroundColor                      ///< 背景色
        radius: 3                                  ///< 圆角半径
        border.width: customButton.hovered ? 1 : 0 ///< 悬停时显示边框
        border.color: customButton.textColor       ///< 边框颜色与文本颜色一致
        opacity: customButton.pressed ? 0.7 : (customButton.hovered ? 0.9 : 1.0) ///< 状态透明度
        
        /// 透明度变化动画
        Behavior on opacity {
            NumberAnimation { duration: 100 }
        }
    }
    
    /**
     * @brief 按钮文本内容
     * 
     * 自定义文本显示，支持居中对齐和焦点状态的粗体效果。
     */
    contentItem: Text {
        text: parent.text                           ///< 显示按钮文本
        color: textColor                            ///< 文本颜色
        font.pixelSize: fontSize                    ///< 字体大小
        horizontalAlignment: Text.AlignHCenter      ///< 水平居中
        verticalAlignment: Text.AlignVCenter        ///< 垂直居中
        font.bold: customButton.focus               ///< 焦点时加粗
    }
    
    /**
     * @brief 悬停效果处理器
     * 
     * 处理鼠标悬停事件，提供视觉反馈。
     */
    HoverHandler {
        id: hoverHandler
    }
}