/**
 * @file BaseDialog.qml
 * @brief 统一的弹窗基础组件
 *
 * 提供所有弹窗的基础样式、属性和行为，确保整个应用程序中
 * 弹窗的一致性和可维护性。所有自定义弹窗都应该继承此组件。
 *
 * @author Sakurakugu
 * @date 未找到提交记录（可能未纳入git）
 * @version 未找到提交记录（可能未纳入git）
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

/**
 * @brief 弹窗基础组件
 *
 * 提供统一的弹窗样式、动画效果和通用属性。
 * 所有自定义弹窗都应该继承此组件以保持一致性。
 */
Dialog {
    id: baseDialog
    
    // 公共属性
    property bool isDarkMode: false                    ///< 深色模式开关
    property string dialogTitle: ""                   ///< 对话框标题
    property int dialogWidth: 300                      ///< 对话框宽度
    property int dialogHeight: 200                     ///< 对话框高度
    property bool showStandardButtons: true            ///< 是否显示标准按钮
    property int standardButtonFlags: Dialog.Ok | Dialog.Cancel  ///< 标准按钮类型
    property alias contentLayout: contentColumn        ///< 内容布局别名
    property int contentMargins: 20                    ///< 内容边距
    property int contentSpacing: 15                    ///< 内容间距
    property bool enableAnimation: true                ///< 是否启用动画
    property real animationDuration: 200               ///< 动画持续时间
    
    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: baseDialog.isDarkMode
    }
    
    // 对话框基本设置
    title: dialogTitle
    modal: true
    anchors.centerIn: parent
    width: dialogWidth
    height: dialogHeight
    standardButtons: showStandardButtons ? standardButtonFlags : Dialog.NoButton
    
    // 统一的背景样式
    background: Rectangle {
        color: theme.backgroundColor
        border.color: theme.borderColor
        border.width: 1
        radius: 8
        
        // 添加阴影效果（使用Qt6原生方式）
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 2
            shadowBlur: 0.8
            shadowColor: theme.shadowColor
        }
    }
    
    // 统一的内容区域
    contentItem: ColumnLayout {
        id: contentColumn
        spacing: contentSpacing
        anchors.margins: contentMargins
        
        // 子类可以在这里添加自定义内容
    }
    
    // 统一的进入动画
    enter: Transition {
        enabled: enableAnimation
        
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: animationDuration
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                property: "scale"
                from: 0.8
                to: 1.0
                duration: animationDuration
                easing.type: Easing.OutBack
                easing.overshoot: 1.2
            }
        }
    }
    
    // 统一的退出动画
    exit: Transition {
        enabled: enableAnimation
        
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: animationDuration * 0.75
                easing.type: Easing.InQuad
            }
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.9
                duration: animationDuration * 0.75
                easing.type: Easing.InQuad
            }
        }
    }
    
    /**
     * @brief 创建统一样式的按钮
     * @param text 按钮文本
     * @param isPrimary 是否为主要按钮
     * @param customColor 自定义颜色（可选）
     * @return Button 组件
     */
    function createStyledButton(text, isPrimary, customColor) {
        var buttonComponent = Qt.createComponent("Button");
        if (buttonComponent.status === Component.Ready) {
            var button = buttonComponent.createObject(baseDialog);
            button.text = text;
            
            // 设置按钮样式
            button.background = Qt.createQmlObject(`
                import QtQuick
                Rectangle {
                    color: {
                        if (parent.pressed) return "${theme.pressedColor}";
                        if (parent.hovered) return "${isPrimary ? theme.primaryColorLight : theme.hoverColor}";
                        return "${customColor || (isPrimary ? theme.primaryColor : theme.secondaryBackgroundColor)}";
                    }
                    border.color: "${theme.borderColor}"
                    border.width: 1
                    radius: 4
                }
            `, button);
            
            button.contentItem = Qt.createQmlObject(`
                import QtQuick
                Text {
                    text: parent.text
                    color: "${isPrimary ? 'white' : theme.textColor}"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 14
                }
            `, button);
            
            return button;
        }
        return null;
    }
    
    /**
     * @brief 创建统一样式的标签
     * @param text 标签文本
     * @param isTitle 是否为标题样式
     * @return Label 组件
     */
    function createStyledLabel(text, isTitle) {
        var labelComponent = Qt.createComponent("Label");
        if (labelComponent.status === Component.Ready) {
            var label = labelComponent.createObject(baseDialog);
            label.text = text;
            label.color = theme.textColor;
            label.wrapMode = Text.WordWrap;
            
            if (isTitle) {
                label.font.pixelSize = 16;
                label.font.bold = true;
            } else {
                label.font.pixelSize = 14;
            }
            
            return label;
        }
        return null;
    }
    
    /**
     * @brief 设置对话框尺寸
     * @param w 宽度
     * @param h 高度
     */
    function setSize(w, h) {
        dialogWidth = w;
        dialogHeight = h;
    }
    
    /**
     * @brief 设置对话框标题
     * @param titleText 标题文本
     */
    function setTitle(titleText) {
        dialogTitle = titleText;
    }
}