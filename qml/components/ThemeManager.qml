/**
 * @file ThemeManager.qml
 * @brief 主题管理组件
 *
 * 该文件定义了一个主题管理组件，用于管理应用程序的主题颜色和样式。
 * 它提供了切换主题、获取当前主题颜色和样式的功能。
 *
 * @author Sakurakugu
 * @date 2025-08-19 07:39:54(UTC+8) 周二
 * @version 2025-08-23 21:09:00(UTC+8) 周六
 */

import QtQuick

QtObject {
    id: themeManager
    
    // 主题状态
    property bool isDarkMode: false
    
    // 主色调
    readonly property color primaryColor: isDarkMode ? "#2c3e50" : "#4a86e8"
    readonly property color primaryColorLight: isDarkMode ? "#34495e" : "#6fa8f5"
    readonly property color primaryColorDark: isDarkMode ? "#1a252f" : "#3d71c4"
    
    // 背景色
    readonly property color backgroundColor: isDarkMode ? "#1e272e" : "white"
    readonly property color secondaryBackgroundColor: isDarkMode ? "#2d3436" : "#f5f5f5"
    readonly property color cardBackgroundColor: isDarkMode ? "#34495e" : "#ffffff"
    
    // 文本色
    readonly property color titleBarTextColor: isDarkMode ? "white" : "#ecf0f1"
    readonly property color textColor: isDarkMode ? "#ecf0f1" : "black"
    readonly property color secondaryTextColor: isDarkMode ? "#bdc3c7" : "#666666"
    readonly property color placeholderTextColor: isDarkMode ? "#7f8c8d" : "#999999"
    
    // 边框和分割线
    readonly property color borderColor: isDarkMode ? "#34495e" : "#cccccc"
    readonly property color separatorColor: isDarkMode ? "#2c3e50" : "#e0e0e0"
    
    // 状态色
    readonly property color successColor: isDarkMode ? "#27ae60" : "#4caf50"
    readonly property color warningColor: isDarkMode ? "#f39c12" : "#ff9800"
    readonly property color errorColor: isDarkMode ? "#e74c3c" : "#f44336"
    readonly property color infoColor: isDarkMode ? "#3498db" : "#2196f3"
    
    // 悬停和选中状态
    readonly property color hoverColor: isDarkMode ? "#34495e" : "#f0f0f0"
    readonly property color selectedColor: isDarkMode ? "#2980b9" : "#e3f2fd"
    readonly property color pressedColor: isDarkMode ? "#1f2937" : "#e8e8e8"
    
    // 按钮状态颜色
    readonly property color buttonHoverColor: isDarkMode ? "#34495e" : "#f0f0f0"
    readonly property color buttonPressedColor: isDarkMode ? "#1f2937" : "#e8e8e8"
    
    // 阴影色
    readonly property color shadowColor: isDarkMode ? "#000000" : "#00000020"
    
    // 透明度变体
    readonly property color overlayColor: isDarkMode ? "#00000080" : "#00000040"
    
    // 任务状态和重要性颜色
    readonly property color completedColor: successColor
    readonly property color highImportantColor: errorColor
    readonly property color mediumImportantColor: warningColor
    readonly property color lowImportantColor: successColor
    
    // 方法：切换主题
    function toggleTheme() {
        isDarkMode = !isDarkMode
    }
    
    // 方法：设置主题
    function setTheme(darkMode) {
        isDarkMode = darkMode
    }
    
    // 方法：获取优先级颜色
    function getPriorityColor(priority) {
        switch(priority) {
            case "高":
                return errorColor
            case "中":
                return warningColor
            case "低":
                return successColor
            default:
                return textColor
        }
    }
    
    // 方法：获取状态颜色
    function getStatusColor(status) {
        switch(status) {
            case "completed":
            case "已完成":
                return successColor
            case "in_progress":
            case "进行中":
                return infoColor
            case "todo":
            case "待办":
                return warningColor
            default:
                return textColor
        }
    }
}