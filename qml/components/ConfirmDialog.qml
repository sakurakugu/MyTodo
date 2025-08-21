/**
 * @file ConfirmDialog.qml
 * @brief 通用确认对话框组件
 *
 * 提供一个可重用的确认对话框，支持自定义消息、按钮文本和主题。
 * 可用于各种需要用户确认的场景，如删除操作、退出登录等。
 *
 * @author Sakurakugu
 * @date 2025
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @brief 通用确认对话框
 *
 * 提供标准的确认对话框界面，包含消息显示和确认/取消按钮。
 * 支持主题切换和自定义按钮文本。
 */
Dialog {
    id: confirmDialog
    
    // 公共属性
    property string message: ""                    ///< 对话框显示的消息内容
    property string yesButtonText: qsTr("确定")     ///< 确认按钮文本
    property string noButtonText: qsTr("取消")      ///< 取消按钮文本
    property bool isDarkMode: false                ///< 深色模式开关
    
    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: confirmDialog.isDarkMode
    }
    
    // 对话框基本设置
    title: qsTr("确认")
    modal: true
    anchors.centerIn: parent
    width: Math.max(300, contentLayout.implicitWidth + 40)
    height: Math.max(150, contentLayout.implicitHeight + 80)
    standardButtons: Dialog.NoButton
    
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
        
        // 消息显示区域
        Label {
            text: confirmDialog.message
            color: theme.textColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.preferredWidth: 260
            horizontalAlignment: Text.AlignHCenter
        }
        
        // 按钮区域
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10
            
            // 取消按钮
            Button {
                text: confirmDialog.noButtonText
                onClicked: confirmDialog.reject()
                
                background: Rectangle {
                    color: parent.pressed ? theme.buttonPressedColor : 
                           parent.hovered ? theme.buttonHoverColor : 
                           theme.secondaryBackgroundColor
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            
            // 确认按钮
            Button {
                text: confirmDialog.yesButtonText
                onClicked: confirmDialog.accept()
                
                background: Rectangle {
                    color: parent.pressed ? theme.buttonPressedColor : 
                           parent.hovered ? theme.buttonHoverColor : 
                           theme.primaryColor
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
    
    /**
     * @brief 打开确认对话框
     * @param msg 要显示的消息
     * @param yesText 确认按钮文本（可选）
     * @param noText 取消按钮文本（可选）
     */
    function openConfirm(msg, yesText, noText) {
        message = msg || "";
        if (yesText) yesButtonText = yesText;
        if (noText) noButtonText = noText;
        open();
    }
}