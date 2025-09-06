/**
 * @file LoginStatusDialogs.qml
 * @brief 登录状态相关对话框组件集合
 *
 * 包含与用户登录状态相关的各种对话框，如登录需要提示、
 * 登录成功、登录失败、退出登录确认等对话框。
 *
 * @author Sakurakugu
 * @date 2025-08-19 07:39:54(UTC+8) 周二
 * @change 2025-09-05 00:31:59(UTC+8) 周五
 * @version 0.4.0
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
    id: root

    anchors.centerIn: parent

    // UserAuth信号连接
    Connections {
        target: userAuth

        function onLoginSuccessful(username) {
            loginDialog.close();
            loginMessageDialog.showMessage(qsTr("欢迎回来，%1！").arg(username));
        }

        function onLoginFailed(errorMessage) {
            loginDialog.setErrorMessage(qsTr("登录失败：%1").arg(errorMessage));
        }

        function onLogoutSuccessful() {
            loginMessageDialog.showMessage(qsTr("已成功退出登录"));
        }
    }

    // 需要登录提示对话框
    ModalDialog {
        id: loginRequiredDialog
        dialogTitle: qsTr("需要登录")
        message: qsTr("开启自动同步功能需要先登录账户。\n请先登录后再开启自动同步。")

        confirmText: qsTr("去登录")

        onCancelled: {
            loginRequiredDialog.close();
        }

        onConfirmed: {
            loginRequiredDialog.close();
            loginDialog.open();
        }
    }

    // 用户登录对话框
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
        contentSpacing: 10  // 减少内容区域间距

        confirmText: loginDialog.isLoggingIn ? qsTr("登录中...") : qsTr("登录")

        isEnableCancelButton: !loginDialog.isLoggingIn
        isEnableConfirmButton: usernameField.text.length > 0 && passwordField.text.length > 0 && !loginDialog.isLoggingIn

        onCancelled: {
            loginDialog.close();
            loginDialog.clearForm();
        }

        onConfirmed: {
            if (usernameField.text.length > 0 && passwordField.text.length > 0) {
                loginDialog.isLoggingIn = true;
                userAuth.login(usernameField.text, passwordField.text);
            } else if (usernameField.text.length == 0 && passwordField.activeFocus) {
                usernameField.forceActiveFocus();
            } else if (passwordField.text.length == 0 && usernameField.activeFocus) {
                passwordField.forceActiveFocus();
            }
        }

        onOpened: {
            usernameField.forceActiveFocus();
        }

        // 文本测量组件（用于计算错误消息高度）
        TextMetrics {
            id: textMetrics
            font.pixelSize: 12
            elide: Text.ElideNone
            elideWidth: 280  // 与对话框宽度保持一致，减去左右边距
        }

        // 错误消息显示区域
        Label {
            text: loginDialog.errorMessage
            color: ThemeManager.errorColor
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
                    color: ThemeManager.textColor
                }

                CustomTextInput {
                    id: usernameField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    placeholderText: qsTr("请输入用户名")
                    enabled: !loginDialog.isLoggingIn // 登录中禁用输入

                    // 文本变化时清除错误消息
                    onTextChanged: {
                        if (loginDialog.errorMessage !== "") {
                            loginDialog.errorMessage = "";
                            loginDialog.maxDialogHeight = 250;
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
                    color: ThemeManager.textColor
                }

                CustomTextInput {
                    id: passwordField
                    placeholderText: qsTr("请输入密码")
                    echoMode: TextInput.Password
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    enabled: !loginDialog.isLoggingIn

                    // 文本变化时清除错误消息
                    onTextChanged: {
                        if (loginDialog.errorMessage !== "") {
                            loginDialog.errorMessage = "";
                            loginDialog.maxDialogHeight = 250;
                        }
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
            resetLoginState();
            errorMessage = message;

            // 根据消息长度动态计算高度
            if (message !== "") {
                // 使用TextMetrics测量文本高度
                textMetrics.text = message;
                var textHeight = textMetrics.boundingRect.height;

                // 基础高度：对话框标题栏、表单区域、按钮区域等固定内容的高度
                var baseHeight = 250;

                // 错误消息区域的额外高度：文本高度 + 上下边距
                var errorAreaHeight = textHeight + 30; // 20px为上下边距

                // 计算总高度，设置最小高度为280，最大高度为400
                var totalHeight = Math.max(280, Math.min(400, baseHeight + errorAreaHeight));
                loginDialog.maxDialogHeight = totalHeight;
            } else {
                // 没有错误消息时使用默认高度
                loginDialog.maxDialogHeight = 250;
            }
        }
    }

    // 退出登录确认对话框
    BaseDialog {
        id: logoutConfirmDialog
        dialogTitle: qsTr("退出登录")
        dialogWidth: 280
        maxDialogHeight: 140
        confirmText: qsTr("确认")
        focus: true

        onCancelled: {
            logoutConfirmDialog.close();
        }

        onConfirmed: {
            logoutConfirmDialog.close();
            userAuth.logout();
            todoManager.deleteAllTodos(deleteAllTodosCheckBox.checked);
        }

        contentSpacing: 10 // 缩小整体竖向间距，便于减小复选框到按钮的距离

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 6
        }

        Label {
            text: qsTr("确认要退出登录吗？")
            color: ThemeManager.textColor
            Layout.alignment: Qt.AlignHCenter
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 6
        }

        CheckBox {
            id: deleteAllTodosCheckBox
            text: qsTr("同时删除所有待办事项")
            checked: false
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 0
            topPadding: 0
            bottomPadding: 0

            indicator: Rectangle {
                implicitWidth: 16
                implicitHeight: 16
                x: deleteAllTodosCheckBox.leftPadding
                y: parent.height / 2 - height / 2
                radius: 2
                border.color: ThemeManager.borderColor
                border.width: 1
                color: deleteAllTodosCheckBox.checked ? ThemeManager.primaryColor : ThemeManager.secondaryBackgroundColor

                Rectangle {
                    width: 8
                    height: 8
                    x: 4
                    y: 4
                    radius: 1
                    color: "white"
                    visible: deleteAllTodosCheckBox.checked
                }
            }

            contentItem: Text {
                text: deleteAllTodosCheckBox.text
                font.pixelSize: 12
                color: "gray" // 灰色
                verticalAlignment: Text.AlignVCenter
                leftPadding: deleteAllTodosCheckBox.indicator.width + deleteAllTodosCheckBox.spacing
            }
        }
    }

    // 登录消息对话框
    ModalDialog {
        id: loginMessageDialog
        dialogTitle: qsTr("登录消息")
        dialogWidth: 280
        dialogHeight: 180
        autoSize: false
        isShowCancelButton: false

        onConfirmed: {
            loginMessageDialog.close();
        }
    }

    // 显示登录需要提示对话框
    function showLoginRequired() {
        loginRequiredDialog.open();
    }

    // 显示登录对话框
    function showLoginDialog() {
        loginDialog.openLogin();
    }

    // 显示退出登录确认对话框
    function showLogoutConfirm() {
        logoutConfirmDialog.open();
    }

    // 重置登录状态
    function resetLoginState() {
        loginDialog.resetLoginState();
    }
}
