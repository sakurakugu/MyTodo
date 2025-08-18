/**
 * @file MessageDialog.qml
 * @brief 通用消息提示对话框组件
 *
 * 提供一个可重用的消息提示对话框，支持不同类型的消息显示，
 * 如成功、错误、警告、信息等，并支持自定义图标和按钮。
 *
 * @author MyTodo Team
 * @date 2024
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @brief 通用消息提示对话框
 *
 * 提供标准的消息提示界面，支持多种消息类型和自定义样式。
 * 可用于显示成功、错误、警告等各种类型的消息。
 */
Dialog {
    id: messageDialog
    
    // 消息类型枚举
    enum MessageType {
        Info,       ///< 信息
        Success,    ///< 成功
        Warning,    ///< 警告
        Error       ///< 错误
    }
    
    // 公共属性
    property string message: ""                           ///< 消息内容
    property int messageType: MessageDialog.MessageType.Info  ///< 消息类型
    property string buttonText: qsTr("确定")              ///< 按钮文本
    property bool isDarkMode: false                       ///< 深色模式开关
    property bool autoClose: false                        ///< 是否自动关闭
    property int autoCloseDelay: 3000                     ///< 自动关闭延迟（毫秒）
    
    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: messageDialog.isDarkMode
    }
    
    // 对话框基本设置
    modal: true
    anchors.centerIn: parent
    width: Math.max(300, contentLayout.implicitWidth + 40)
    height: Math.max(150, contentLayout.implicitHeight + 80)
    standardButtons: Dialog.NoButton
    
    // 根据消息类型设置标题
    title: {
        switch (messageType) {
            case MessageDialog.MessageType.Success:
                return qsTr("成功");
            case MessageDialog.MessageType.Warning:
                return qsTr("警告");
            case MessageDialog.MessageType.Error:
                return qsTr("错误");
            default:
                return qsTr("提示");
        }
    }
    
    // 背景样式
    background: Rectangle {
        color: theme.backgroundColor
        border.color: theme.borderColor
        border.width: 1
        radius: 8
    }
    
    // 内容区域
    contentItem: ColumnLayout {
        id: contentLayout
        spacing: 20
        anchors.margins: 20
        
        // 图标和消息区域
        RowLayout {
            Layout.fillWidth: true
            spacing: 15
            
            // 消息类型图标
            Text {
                text: {
                    switch (messageDialog.messageType) {
                        case MessageDialog.MessageType.Success:
                            return "✅";
                        case MessageDialog.MessageType.Warning:
                            return "⚠️";
                        case MessageDialog.MessageType.Error:
                            return "❌";
                        default:
                            return "ℹ️";
                    }
                }
                font.pixelSize: 24
                Layout.alignment: Qt.AlignTop
            }
            
            // 消息文本
            Label {
                text: messageDialog.message
                color: theme.textColor
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.preferredWidth: 220
                verticalAlignment: Text.AlignVCenter
            }
        }
        
        // 按钮区域
        RowLayout {
            Layout.alignment: Qt.AlignRight
            
            Button {
                text: messageDialog.buttonText
                onClicked: messageDialog.close()
                
                background: Rectangle {
                    color: {
                        if (parent.pressed) return theme.buttonPressedColor;
                        if (parent.hovered) return theme.buttonHoverColor;
                        
                        // 根据消息类型设置按钮颜色
                        switch (messageDialog.messageType) {
                            case MessageDialog.MessageType.Success:
                                return "#4CAF50";  // 绿色
                            case MessageDialog.MessageType.Warning:
                                return "#FF9800";  // 橙色
                            case MessageDialog.MessageType.Error:
                                return "#F44336";  // 红色
                            default:
                                return theme.primaryColor;
                        }
                    }
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
    
    // 自动关闭定时器
    Timer {
        id: autoCloseTimer
        interval: messageDialog.autoCloseDelay
        running: false
        repeat: false
        onTriggered: messageDialog.close()
    }
    
    // 对话框打开时的处理
    onOpened: {
        if (autoClose) {
            autoCloseTimer.start();
        }
    }
    
    // 对话框关闭时的处理
    onClosed: {
        autoCloseTimer.stop();
    }
    
    /**
     * @brief 显示信息消息
     * @param msg 消息内容
     * @param btnText 按钮文本（可选）
     * @param autoCloseEnabled 是否自动关闭（可选）
     */
    function showInfo(msg, btnText, autoCloseEnabled) {
        messageType = MessageDialog.MessageType.Info;
        message = msg || "";
        buttonText = btnText || qsTr("确定");
        autoClose = autoCloseEnabled || false;
        open();
    }
    
    /**
     * @brief 显示成功消息
     * @param msg 消息内容
     * @param btnText 按钮文本（可选）
     * @param autoCloseEnabled 是否自动关闭（可选）
     */
    function showSuccess(msg, btnText, autoCloseEnabled) {
        messageType = MessageDialog.MessageType.Success;
        message = msg || "";
        buttonText = btnText || qsTr("确定");
        autoClose = autoCloseEnabled || false;
        open();
    }
    
    /**
     * @brief 显示警告消息
     * @param msg 消息内容
     * @param btnText 按钮文本（可选）
     * @param autoCloseEnabled 是否自动关闭（可选）
     */
    function showWarning(msg, btnText, autoCloseEnabled) {
        messageType = MessageDialog.MessageType.Warning;
        message = msg || "";
        buttonText = btnText || qsTr("确定");
        autoClose = autoCloseEnabled || false;
        open();
    }
    
    /**
     * @brief 显示错误消息
     * @param msg 消息内容
     * @param btnText 按钮文本（可选）
     * @param autoCloseEnabled 是否自动关闭（可选）
     */
    function showError(msg, btnText, autoCloseEnabled) {
        messageType = MessageDialog.MessageType.Error;
        message = msg || "";
        buttonText = btnText || qsTr("确定");
        autoClose = autoCloseEnabled || false;
        open();
    }
}