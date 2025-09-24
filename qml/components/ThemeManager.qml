/**
 * @file ThemeManager.qml
 * @brief 主题管理组件（单例）
 *
 * 该文件定义了一个主题管理组件，用于管理应用程序的主题颜色和样式。
 * 它提供了切换主题、获取当前主题颜色和样式的功能。
 *
 * @author Sakurakugu
 * @date 2025-08-17 07:17:29(UTC+8) 周日
 * @change 2025-09-10 01:01:04(UTC+8) 周三
 */
pragma Singleton
import QtQuick

QtObject {
    id: root

    // 主题状态
    property bool isDarkMode: globalState.isDarkMode

    // 动画配置
    readonly property int colorAnimationDuration: 150    ///< 颜色动画持续时间（毫秒）
    readonly property int opacityAnimationDuration: 100  ///< 透明度动画持续时间（毫秒）
    readonly property int positionAnimationDuration: 200 ///< 位置动画持续时间（毫秒）

    // 主色调
    readonly property color primaryColor: isDarkMode ? "#2c3e50" : "#4a86e8"
    readonly property color primaryColorLight: isDarkMode ? "#34495e" : "#6fa8f5"
    readonly property color primaryColorDark: isDarkMode ? "#1a252f" : "#3d71c4"

    // 背景色
    readonly property color backgroundColor: isDarkMode ? "#1e272e" : "white"
    readonly property color secondaryBackgroundColor: isDarkMode ? "#2d3436" : "#f5f5f5"
    readonly property color cardBackgroundColor: isDarkMode ? "#34495e" : "#ffffff"

    // 文本色
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

    // 主按钮颜色（蓝色系）
    readonly property color buttonColor: isDarkMode ? "#1e3a8a" : "#3b82f6" // 按钮颜色
    readonly property color buttonHoverColor: isDarkMode ? "#1d4ed8" : "#2563eb" // 悬停颜色
    readonly property color buttonPressedColor: isDarkMode ? "#1e40af" : "#1d4ed8" // 按下颜色
    readonly property color buttonDisabledColor: isDarkMode ? "#374151" : "#a8b1c0" // 禁用颜色
    readonly property color buttonTextColor: isDarkMode ? "#f8fafc" : "white" // 按钮文本颜色
    readonly property color buttonDisabledTextColor: isDarkMode ? "#6b7280" : "#d1d5db" // 禁用按钮文本颜色

    // 副按钮颜色（灰色系）
    readonly property color button2Color: isDarkMode ? "#374151" : "#f3f4f6" // 按钮颜色
    readonly property color button2HoverColor: isDarkMode ? "#4b5563" : "#e5e7eb" // 悬停颜色
    readonly property color button2PressedColor: isDarkMode ? "#1f2937" : "#d1d5db" // 按下颜色
    readonly property color button2DisabledColor: isDarkMode ? "#1f2937" : "#f9fafb" // 禁用颜色
    readonly property color button2TextColor: isDarkMode ? "#d1d5db" : "#374151" // 按钮文本颜色
    readonly property color button2DisabledTextColor: isDarkMode ? "#6b7280" : "#9ca3af" // 禁用按钮文本颜色

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
        isDarkMode = !isDarkMode;
    }

    // 方法：设置主题
    function setTheme(darkMode) {
        isDarkMode = darkMode;
    }
}
