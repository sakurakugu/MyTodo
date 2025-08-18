/**
 * @file LoginDialog.qml
 * @brief 用户登录对话框组件
 *
 * 提供用户登录界面，包含用户名和密码输入框，
 * 以及登录状态处理和错误提示功能。
 *
 * @author MyTodo Team
 * @date 2024
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @brief 用户登录对话框
 *
 * 提供完整的用户登录界面，包含表单验证、登录状态显示等功能。
 * 支持主题切换和响应式布局。
 */
Dialog {
    id: loginDialog
    
    // 公共属性
    property alias username: usernameField.text    ///< 用户名
    property alias password: passwordField.text    ///< 密码
    property bool isLoggingIn: false               ///< 登录状态
    property bool isDarkMode: false                ///< 深色模式开关
    
    // 信号
    signal loginRequested(string username, string password)  ///< 登录请求信号
    
    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: loginDialog.isDarkMode
    }
    
    // 对话框基本设置
    title: qsTr("用户登录")
    modal: true
    anchors.centerIn: parent
    width: 350
    height: 280
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
        spacing: 20
        anchors.margins: 20
        
        // 标题区域
        Label {
            text: qsTr("请输入您的登录信息")
            color: theme.textColor
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
        }
        
        // 表单区域
        ColumnLayout {
            spacing: 15
            Layout.fillWidth: true
            
            // 用户名输入
            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                
                Label {
                    text: qsTr("用户名:")
                    color: theme.textColor
                }
                
                TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: qsTr("请输入用户名")
                    color: theme.textColor
                    enabled: !loginDialog.isLoggingIn
                    
                    // 回车键处理
                    Keys.onReturnPressed: {
                        if (passwordField.text.length > 0) {
                            loginButton.clicked();
                        } else {
                            passwordField.forceActiveFocus();
                        }
                    }
                }
            }
            
            // 密码输入
            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                
                Label {
                    text: qsTr("密码:")
                    color: theme.textColor
                }
                
                TextField {
                    id: passwordField
                    placeholderText: qsTr("请输入密码")
                    echoMode: TextInput.Password
                    Layout.fillWidth: true
                    color: theme.textColor
                    enabled: !loginDialog.isLoggingIn
                    
                    // 回车键处理
                    Keys.onReturnPressed: {
                        if (usernameField.text.length > 0 && passwordField.text.length > 0) {
                            loginButton.clicked();
                        }
                    }
                }
            }
        }
        
        // 按钮区域
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10
            
            // 取消按钮
            Button {
                text: qsTr("取消")
                enabled: !loginDialog.isLoggingIn
                onClicked: {
                    loginDialog.close();
                    clearForm();
                }
                
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
            
            // 登录按钮
            Button {
                id: loginButton
                text: loginDialog.isLoggingIn ? qsTr("登录中...") : qsTr("登录")
                enabled: usernameField.text.length > 0 && 
                        passwordField.text.length > 0 && 
                        !loginDialog.isLoggingIn
                
                onClicked: {
                    loginDialog.isLoggingIn = true;
                    loginDialog.loginRequested(usernameField.text, passwordField.text);
                }
                
                background: Rectangle {
                    color: parent.enabled ? 
                           (parent.pressed ? theme.buttonPressedColor : 
                            parent.hovered ? theme.buttonHoverColor : 
                            theme.primaryColor) : 
                           theme.borderColor
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "white" : theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
    
    /**
     * @brief 清空表单
     */
    function clearForm() {
        usernameField.text = "";
        passwordField.text = "";
        isLoggingIn = false;
    }
    
    /**
     * @brief 重置登录状态
     */
    function resetLoginState() {
        isLoggingIn = false;
    }
    
    /**
     * @brief 打开登录对话框
     */
    function openLogin() {
        clearForm();
        open();
        usernameField.forceActiveFocus();
    }
}