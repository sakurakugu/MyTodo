/**
 * @file LoginStatusDialogs.qml
 * @brief 登录状态相关对话框组件集合
 *
 * 包含与用户登录状态相关的各种对话框，如登录需要提示、
 * 登录成功、登录失败、退出登录确认等对话框。
 *
 * @author Sakurakugu
 * @date 2025-08-19 07:39:54(UTC+8) 周二
 * @version 2025-08-23 21:09:00(UTC+8) 周六
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

    // 信号定义
    signal loginSuccessMessage(string message)  ///< 登录成功消息信号
    signal logoutSuccessMessage(string message) ///< 退出登录成功消息信号
    signal logoutConfirmed                      ///< 退出登录确认信号

    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: loginStatusDialogs.isDarkMode
    }

    anchors.centerIn: parent

    // UserAuth信号连接
    Connections {
        target: userAuth

        function onLoginSuccessful(username) {
            loginDialog.close();
        // 因为会发送两次信号，不知道为什么，所以这里注释掉
        // loginMessageDialog.showMessage(qsTr("欢迎回来，%1！").arg(username));
        }

        function onLoginFailed(errorMessage) {
            loginDialog.setErrorMessage(qsTr("登录失败：%1").arg(errorMessage));
        }

        function onLogoutSuccessful() {
            loginMessageDialog.showMessage(qsTr("已成功退出登录"));
        }
    }

    /**
     * @brief 需要登录提示对话框
     *
     * 当用户尝试使用需要登录的功能时显示此对话框。
     */
    BaseDialog {
        id: loginRequiredDialog
        dialogTitle: qsTr("需要登录")
        dialogWidth: 280
        maxDialogHeight: 140
        showStandardButtons: false
        isDarkMode: loginStatusDialogs.isDarkMode

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Label {
            text: qsTr("开启自动同步功能需要先登录账户。\n请先登录后再开启自动同步。")
            wrapMode: Text.WordWrap
            color: theme.textColor
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10

            Button {
                text: qsTr("取消")
                onClicked: loginRequiredDialog.close()

                background: Rectangle {
                    color: parent.pressed ? theme.buttonPressedColor : parent.hovered ? theme.buttonHoverColor : theme.secondaryBackgroundColor
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: theme.textColor
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                text: qsTr("去登录")
                onClicked: {
                    loginRequiredDialog.close();
                    loginDialog.open();
                }

                background: Rectangle {
                    color: parent.pressed ? theme.buttonPressedColor : parent.hovered ? theme.buttonHoverColor : theme.primaryColor
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    /**
    * @brief 用户登录对话框
    *
    * 提供完整的用户登录界面，包含表单验证、登录状态显示等功能。
    * 支持主题切换和响应式布局。
    */
    BaseDialog {
        id: loginDialog

        // 公共属性
        property alias username: usernameField.text    ///< 用户名
        property alias password: passwordField.text    ///< 密码
        property bool isLoggingIn: false               ///< 登录状态
        property string errorMessage: ""               ///< 错误消息

        // 对话框设置
        dialogTitle: qsTr("用户登录")
        dialogWidth: 320
        dialogHeight: 250
        maxDialogHeight: 250
        showStandardButtons: false
        contentSpacing: 10  // 减少内容区域间距
        isDarkMode: loginStatusDialogs.isDarkMode

        // 错误消息显示区域
        Label {
            id: errorLabel
            text: loginDialog.errorMessage
            color: theme.errorColor
            font.pixelSize: 12
            Layout.alignment: Qt.AlignHCenter         // 组件居中对齐
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter    // 文本居中对齐
            wrapMode: Text.WordWrap                   // 自动换行
            visible: loginDialog.errorMessage !== ""
        }

        // 表单区域
        ColumnLayout {
            spacing: 10 // 组件间距
            Layout.fillWidth: true

            // 用户名输入
            ColumnLayout {
                spacing: 10
                Layout.fillWidth: true

                Label {
                    text: qsTr("用户名:")
                    color: theme.textColor
                }

                TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    placeholderText: qsTr("请输入用户名")
                    color: theme.textColor
                    enabled: !loginDialog.isLoggingIn // 登录中禁用输入

                    // 文本变化时清除错误消息
                    onTextChanged: {
                        if (loginDialog.errorMessage !== "") {
                            loginDialog.errorMessage = "";
                            setMaxHeight(250);
                        }
                    }

                    // 回车键处理（支持主键盘和小键盘回车）
                    Keys.onReturnPressed: {
                        if (passwordField.text.length > 0) {
                            loginButton.clicked();
                        } else {
                            passwordField.forceActiveFocus();
                        }
                    }
                    Keys.onEnterPressed: {
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
                spacing: 10
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
                    Layout.preferredHeight: 36
                    color: theme.textColor
                    enabled: !loginDialog.isLoggingIn

                    // 文本变化时清除错误消息
                    onTextChanged: {
                        if (loginDialog.errorMessage !== "") {
                            loginDialog.errorMessage = "";
                            setMaxHeight(250);
                        }
                    }

                    // 回车键处理（支持主键盘和小键盘回车）
                    Keys.onReturnPressed: {
                        if (usernameField.text.length > 0 && passwordField.text.length > 0) {
                            loginButton.clicked();
                        } else if (usernameField.text.length == 0) {
                            usernameField.forceActiveFocus();
                        }
                    }
                    Keys.onEnterPressed: {
                        if (usernameField.text.length > 0 && passwordField.text.length > 0) {
                            loginButton.clicked();
                        } else if (usernameField.text.length == 0) {
                            usernameField.forceActiveFocus();
                        }
                    }
                }
            }
        }

        // 按钮区域
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 20

            // 取消按钮
            Button {
                text: qsTr("取消")
                enabled: !loginDialog.isLoggingIn
                onClicked: {
                    loginDialog.close();
                    loginDialog.clearForm();
                }

                background: Rectangle {
                    color: parent.pressed ? theme.buttonPressedColor : parent.hovered ? theme.buttonHoverColor : theme.secondaryBackgroundColor
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 8
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
                enabled: usernameField.text.length > 0 && passwordField.text.length > 0 && !loginDialog.isLoggingIn

                onClicked: {
                    loginDialog.isLoggingIn = true;
                    userAuth.login(usernameField.text, passwordField.text);
                }

                background: Rectangle {
                    color: parent.enabled ? (parent.pressed ? theme.buttonPressedColor : parent.hovered ? theme.buttonHoverColor : theme.primaryColor) : theme.borderColor
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 8
                }

                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "white" : theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
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
            errorMessage = "";
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

        /**
        * @brief 设置错误消息
        * @param message 错误消息文本
        */
        function setErrorMessage(message) {
            setMaxHeight(280);
            resetLoginState();
            errorMessage = message;
        }
    }

    /**
      * @brief 退出登录确认对话框
      *
      * 当用户点击退出登录时显示确认对话框。
      */
    BaseDialog {
        id: logoutConfirmDialog
        dialogTitle: qsTr("退出登录")
        dialogWidth: 280
        maxDialogHeight: 140
        showStandardButtons: false
        isDarkMode: loginStatusDialogs.isDarkMode

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Label {
            text: qsTr("确定要退出登录吗？")
            color: theme.textColor
            Layout.alignment: Qt.AlignHCenter
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10

            Button {
                text: qsTr("取消")
                onClicked: logoutConfirmDialog.close()

                background: Rectangle {
                    color: parent.pressed ? theme.buttonPressedColor : parent.hovered ? theme.buttonHoverColor : theme.secondaryBackgroundColor
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: theme.textColor
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                text: qsTr("确定")
                onClicked: {
                    logoutConfirmDialog.close();
                    loginStatusDialogs.logoutConfirmed();
                    userAuth.logout();
                }

                background: Rectangle {
                    color: parent.pressed ? theme.buttonPressedColor : parent.hovered ? theme.buttonHoverColor : theme.primaryColor
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    /**
     * @brief 登录消息对话框
     *
     * 当用户登录成功或失败显示该对话框
     */
    BaseDialog {
        id: loginMessageDialog
        dialogTitle: qsTr("登录消息")
        dialogWidth: 280
        dialogHeight: 180
        // maxDialogHeight: 100
        autoSize: false
        showStandardButtons: false
        isDarkMode: loginStatusDialogs.isDarkMode

        property string message: ""

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Label {
            text: qsTr(loginMessageDialog.message)
            wrapMode: Text.WordWrap
            color: theme.textColor
            Layout.fillWidth: true
            Layout.topMargin: 10
            horizontalAlignment: Text.AlignHCenter
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 5

            Button {
                text: qsTr("确定")
                onClicked: {
                    loginMessageDialog.close();
                }

                background: Rectangle {
                    color: parent.pressed ? theme.buttonPressedColor : parent.hovered ? theme.buttonHoverColor : theme.primaryColor
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        function showMessage(message) {
            loginMessageDialog.message = message;
            loginMessageDialog.open();
        }
    }

    /**
     * @brief 显示登录需要提示对话框
     */
    function showLoginRequired() {
        loginRequiredDialog.open();
    }

    /**
     * @brief 显示登录对话框
     */
    function showLoginDialog() {
        loginDialog.openLogin();
    }

    /**
     * @brief 显示退出登录确认对话框
     */
    function showLogoutConfirm() {
        logoutConfirmDialog.open();
    }

    /**
     * @brief 重置登录状态
     */
    function resetLoginState() {
        loginDialog.resetLoginState();
    }
}
