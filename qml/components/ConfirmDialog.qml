/**
 * @file ConfirmDialog.qml
 * @brief 通用确认对话框组件
 *
 * 提供一个可重用的确认对话框，支持自定义消息、按钮文本和主题。
 * 可用于各种需要用户确认的场景，如删除操作、退出登录等。
 *
 * @author Sakurakugu
 * @date 2025-08-19 07:39:54(UTC+8) 周二
 * @version 2025-08-21 21:31:41(UTC+8) 周四
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
BaseDialog {
    id: confirmDialog
    
    // 公共属性
    property string message: ""                    ///< 对话框显示的消息内容
    property string yesButtonText: qsTr("确定")     ///< 确认按钮文本
    property string noButtonText: qsTr("取消")      ///< 取消按钮文本
    
    // 对话框设置
    dialogTitle: qsTr("确认")
    dialogWidth: Math.max(300, messageLabel.implicitWidth + 80)
    dialogHeight: Math.max(150, contentLayout.implicitHeight + 100)
    showStandardButtons: false
    
    // 消息文本
    Label {
        id: messageLabel
        text: confirmDialog.message
        color: theme.textColor
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        Layout.preferredWidth: 220
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
                color: parent.pressed ? theme.pressedColor : 
                       parent.hovered ? theme.hoverColor : 
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
                font.pixelSize: 14
            }
        }
        
        // 确认按钮
        Button {
            text: confirmDialog.yesButtonText
            onClicked: confirmDialog.accept()
            
            background: Rectangle {
                color: parent.pressed ? theme.pressedColor : 
                       parent.hovered ? theme.primaryColorLight : 
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
                font.pixelSize: 14
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