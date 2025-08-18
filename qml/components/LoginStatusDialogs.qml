/**
 * @file LoginStatusDialogs.qml
 * @brief 登录状态相关对话框组件集合
 *
 * 包含与用户登录状态相关的各种对话框，如登录需要提示、
 * 登录成功、登录失败、退出登录确认等对话框。
 *
 * @author MyTodo Team
 * @date 2024
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @brief 登录状态对话框容器
 *
 * 提供与登录状态相关的所有对话框组件，
 * 统一管理登录流程中的各种提示和确认对话框。
 */
Item {
    id: loginStatusDialogs
    
    // 公共属性
    property bool isDarkMode: false  ///< 深色模式开关
    
    // 信号
    signal loginRequested()          ///< 请求登录信号
    signal logoutConfirmed()         ///< 确认退出登录信号
    
    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: loginStatusDialogs.isDarkMode
    }
    
    anchors.centerIn: parent
    
    /**
     * @brief 登录需要提示对话框
     *
     * 当用户尝试使用需要登录的功能时显示此对话框。
     */
    Dialog {
        id: loginRequiredDialog
        title: qsTr("需要登录")
        modal: true
        anchors.centerIn: parent
        width: 300
        height: 180
        standardButtons: Dialog.NoButton
        
        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 8
        }
        
        contentItem: ColumnLayout {
            spacing: 20
            anchors.margins: 20
            
            Label {
                text: qsTr("开启自动同步功能需要先登录账户。\n请先登录后再开启自动同步。")
                wrapMode: Text.WordWrap
                color: theme.textColor
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
            
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10
                
                Button {
                    text: qsTr("取消")
                    onClicked: loginRequiredDialog.close()
                    
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
                
                Button {
                    text: qsTr("去登录")
                    onClicked: {
                        loginRequiredDialog.close();
                        loginStatusDialogs.loginRequested();
                    }
                    
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
    }
    
    /**
     * @brief 退出登录确认对话框
     *
     * 当用户点击退出登录时显示确认对话框。
     */
    Dialog {
        id: logoutConfirmDialog
        title: qsTr("确认退出")
        modal: true
        anchors.centerIn: parent
        width: 300
        height: 150
        standardButtons: Dialog.NoButton
        
        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 8
        }
        
        contentItem: ColumnLayout {
            spacing: 20
            anchors.margins: 20
            
            Label {
                text: qsTr("确定要退出登录吗？")
                color: theme.textColor
                Layout.alignment: Qt.AlignHCenter
            }
            
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10
                
                Button {
                    text: qsTr("取消")
                    onClicked: logoutConfirmDialog.close()
                    
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
                
                Button {
                    text: qsTr("确定")
                    onClicked: {
                        logoutConfirmDialog.close();
                        loginStatusDialogs.logoutConfirmed();
                    }
                    
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
    }
    
    // 公共方法
    
    /**
     * @brief 显示登录需要提示对话框
     */
    function showLoginRequired() {
        loginRequiredDialog.open();
    }
    
    /**
     * @brief 显示退出登录确认对话框
     */
    function showLogoutConfirm() {
        logoutConfirmDialog.open();
    }
    
    /**
     * @brief 关闭所有对话框
     */
    function closeAll() {
        loginRequiredDialog.close();
        logoutConfirmDialog.close();
    }
}