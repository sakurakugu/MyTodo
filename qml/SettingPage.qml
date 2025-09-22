/**
 * @brief 设置页面组件
 *
 * 该组件用于创建应用的设置页面，包含应用的代理设置、主题设置、关于信息等。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-06 01:29:53(UTC+8) 周六
 * @version 0.4.0
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import "./components"

Page {
    id: settingPage

    property var root
    property var stackView
    property var loginStatusDialogs

    // 应用代理设置函数
    function applyProxySettings() {
        if (!setting.getProxyEnabled()) {
            // 禁用代理
            setting.setProxyConfig(false, 0, "", 0, "", ""); // NoProxy
        } else {
            var proxyType = setting.getProxyType();
            var host = setting.getProxyHost();
            var port = setting.getProxyPort();
            var username = setting.getProxyUsername();
            var password = setting.getProxyPassword();

            setting.setProxyConfig(true, proxyType, host, port, username, password);
        }
    }

    background: MainBackground {}

    // 标题栏
    header: TitleBar {
        title: qsTr("设置")
        showBackButton: true
        targetWindow: settingPage.root
        stackView: settingPage.stackView
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.fill: parent
            anchors.topMargin: 20
            anchors.leftMargin: parent.width * 0.2
            anchors.rightMargin: parent.width * 0.2
            spacing: 15

            // 个人信息
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    id: userProfileContent
                    spacing: 10

                    // 显示用户头像
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: Math.min(width, height) / 2       ///< 圆形头像
                        color: "white"                            ///< 不改变颜色
                        Layout.alignment: Qt.AlignVCenter         ///< 垂直居中对齐
                        border.width: 1
                        border.color: ThemeManager.borderColor

                        /// 头像图标
                        Text {
                            anchors.centerIn: parent
                            text: "👤"                                ///< 默认用户图标
                            font.pixelSize: 18
                        }
                    }

                    // 显示用户名
                    Text {
                        text: userAuth.username !== "" ? userAuth.username : qsTr("未登录")
                        color: ThemeManager.textColor      ///< 使用主题文本颜色
                        font.bold: true                     ///< 粗体字
                        font.pixelSize: 16                  ///< 字体大小
                        Layout.alignment: Qt.AlignVCenter   ///< 垂直居中对齐
                        horizontalAlignment: Text.AlignLeft ///< 水平左对齐
                    }
                }
            }

            Divider {}

            Layout.fillWidth: true

            Label {
                text: qsTr("外观设置")
                font.bold: true
                font.pixelSize: 16
                color: ThemeManager.textColor
            }

            ControlRow {
                id: darkModeCheckBox
                text: qsTr("深色模式")
                checked: globalState.isDarkMode
                enabled: !globalState.isFollowSystemDarkMode
                leftMargin: 10
                controlType: ControlRow.ControlType.Switch

                onCheckedChanged: {
                    globalState.isDarkMode = checked;
                    // 保存设置到配置文件
                    setting.save("setting/isDarkMode", checked);
                }
            }

            ControlRow {
                id: followSystemThemeCheckBox
                text: qsTr("跟随系统深色模式")
                checked: setting.get("setting/followSystemTheme", false)
                leftMargin: 10
                controlType: ControlRow.ControlType.Switch

                Component.onCompleted: {
                    if (checked) {
                        globalState.isFollowSystemDarkMode = checked;
                        // 如果启用跟随系统，立即同步系统主题
                        if (globalState.isDarkMode !== globalState.isSystemInDarkMode) {
                            globalState.isDarkMode = globalState.isSystemInDarkMode;
                        }
                    }
                }

                onCheckedChanged: {
                    setting.save("setting/followSystemTheme", checked);
                    globalState.isFollowSystemDarkMode = checked;

                    if (checked) {
                        // 启用跟随系统时，立即同步系统主题
                        if (globalState.isDarkMode !== globalState.isSystemInDarkMode) {
                            globalState.isDarkMode = globalState.isSystemInDarkMode;
                        }
                    }
                }

                // 监听系统主题变化
                Connections {
                    target: globalState
                    function onSystemInDarkModeChanged() {
                        if (followSystemThemeCheckBox.checked) {
                            if (globalState.isDarkMode !== globalState.isSystemInDarkMode) {
                                globalState.isDarkMode = globalState.isSystemInDarkMode;
                            }
                        }
                    }
                }
            }

            ControlRow {
                id: preventDraggingCheckBox
                text: qsTr("防止拖动窗口（小窗口模式）")
                checked: globalState.preventDragging
                enabled: globalState.isDesktopWidget
                leftMargin: 10
                controlType: ControlRow.ControlType.Switch
                onCheckedChanged: {
                    globalState.preventDragging = checked;
                    setting.save("setting/preventDragging", globalState.preventDragging);
                }
            }

            ControlRow {
                id: autoStartSwitch
                text: qsTr("开机自启动")
                checked: globalState.isAutoStartEnabled()
                leftMargin: 10
                controlType: ControlRow.ControlType.Switch
                onCheckedChanged: {
                    globalState.setAutoStart(checked);
                }
            }

            ControlRow {
                text: userAuth.isLoggedIn ? qsTr("自动同步") : qsTr("自动同步（未登录）")
                checked: globalState.isAutoSyncEnabled
                leftMargin: 10
                controlType: ControlRow.ControlType.Switch

                onCheckedChanged: {
                    if (checked) {
                        // 如果未登录，显示提示并重置开关
                        if (!userAuth.isLoggedIn) {
                            toggle();
                            settingPage.loginStatusDialogs.showLoginRequired();
                        } else {
                            globalState.setIsAutoSyncEnabled(checked);
                        }
                    }
                }
            }

            Divider {}

            Label {
                text: qsTr("网络代理设置")
                font.bold: true
                font.pixelSize: 16
                color: ThemeManager.textColor
            }

            ControlRow {
                id: proxyEnabledSwitch
                text: qsTr("启用代理")
                checked: setting.getProxyEnabled()
                leftMargin: 10
                controlType: ControlRow.ControlType.Switch

                onCheckedChanged: {
                    setting.setProxyEnabled(checked);
                    // 应用代理设置
                    applyProxySettings();
                }
            }

            CustomComboBox {
                id: proxyTypeCombo
                enabled: proxyEnabledSwitch.checked
                visible: proxyEnabledSwitch.checked
                Layout.leftMargin: 20
                model: [qsTr("不使用代理"), qsTr("系统代理"), qsTr("HTTP代理"), qsTr("SOCKS5代理")]
                currentIndex: setting.getProxyType()

                onCurrentIndexChanged: {
                    setting.setProxyType(currentIndex);
                    applyProxySettings();
                }
            }

            // 代理服务器设置（仅在选择HTTP或SOCKS5代理时显示）
            Column {
                visible: proxyEnabledSwitch.checked && (proxyTypeCombo.currentIndex === 2 || proxyTypeCombo.currentIndex === 3)
                spacing: 10
                width: parent.width

                Row {
                    spacing: 10
                    width: parent.width

                    Label {
                        text: qsTr("服务器:")
                        width: 80
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    CustomTextInput {
                        id: proxyHostField
                        placeholderText: qsTr("代理服务器地址")
                        text: setting.getProxyHost()
                        width: parent.width - 90

                        onTextChanged: {
                            setting.setProxyHost(text);
                            applyProxySettings();
                        }
                    }
                }

                Row {
                    spacing: 10
                    width: parent.width

                    Label {
                        text: qsTr("端口:")
                        width: 80
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    CustomSpinBox {
                        id: proxyPortSpinBox
                        from: 1
                        to: 65535
                        editable: true
                        value: setting.getProxyPort()

                        onValueChanged: {
                            setting.setProxyPort(value);
                            applyProxySettings();
                        }
                    }
                }

                Row {
                    spacing: 10
                    width: parent.width

                    Label {
                        text: qsTr("用户名:")
                        width: 80
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    CustomTextInput {
                        id: proxyUsernameField
                        placeholderText: qsTr("用户名（可选）")
                        text: setting.getProxyUsername()
                        width: parent.width - 90

                        onTextChanged: {
                            setting.setProxyUsername(text);
                            applyProxySettings();
                        }
                    }
                }

                Row {
                    spacing: 10
                    width: parent.width

                    Label {
                        text: qsTr("密码:")
                        width: 80
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    CustomTextInput {
                        id: proxyPasswordField
                        placeholderText: qsTr("密码（可选）")
                        text: setting.getProxyPassword()
                        echoMode: TextInput.Password
                        width: parent.width - 90

                        onTextChanged: {
                            setting.setProxyPassword(text);
                            applyProxySettings();
                        }
                    }
                }
            }

            Divider {}

            Label {
                text: qsTr("数据管理")
                font.bold: true
                font.pixelSize: 16
                color: ThemeManager.textColor
                Layout.topMargin: 10
            }

            RowLayout {
                spacing: 10
                Layout.leftMargin: 20
                CustomButton {
                    text: qsTr("导出待办事项")

                    backgroundColor: ThemeManager.successColor
                    hoverColor: Qt.darker(ThemeManager.successColor, 1.1)
                    pressedColor: Qt.darker(ThemeManager.successColor, 1.2)
                    onClicked: exportFileDialog.open()
                }

                CustomButton {
                    text: qsTr("导入待办事项")
                    onClicked: importFileDialog.open()
                }
            }

            Divider {}

            Label {
                text: qsTr("配置文件管理")
                font.bold: true
                font.pixelSize: 16
                color: ThemeManager.textColor
                Layout.topMargin: 20
            }

            ColumnLayout {
                spacing: 10
                Layout.fillWidth: true
                Layout.leftMargin: 20

                // 配置文件路径显示（仅对文件类型显示）

                Label {
                    text: qsTr("配置文件路径:")
                    color: ThemeManager.textColor
                }

                CustomTextInput {
                    id: configPathField
                    Layout.fillWidth: true
                    text: setting.getConfigFilePath()
                    readOnly: true
                }

                RowLayout {
                    spacing: 10

                    CustomButton {
                        text: qsTr("打开目录")
                        backgroundColor: ThemeManager.successColor
                        hoverColor: Qt.darker(ThemeManager.successColor, 1.1)
                        pressedColor: Qt.darker(ThemeManager.successColor, 1.2)
                        onClicked: {
                            if (!setting.openConfigFilePath()) {
                                modalDialog.showInfo(qsTr("操作失败"), qsTr("无法打开配置文件目录"));
                            }
                        }
                    }

                    CustomButton {
                        text: qsTr("清空配置")
                        backgroundColor: ThemeManager.errorColor
                        hoverColor: Qt.darker(ThemeManager.errorColor, 1.1)
                        pressedColor: Qt.darker(ThemeManager.errorColor, 1.2)
                        onClicked: clearConfigDialog.open()
                    }
                }
            }

            Divider {}

            Label {
                text: qsTr("服务器配置")
                font.bold: true
                font.pixelSize: 16
                color: ThemeManager.textColor
                Layout.topMargin: 10
            }

            ColumnLayout {
                spacing: 10
                Layout.fillWidth: true
                Layout.leftMargin: 20

                Label {
                    text: qsTr("API服务器地址:")
                    color: ThemeManager.textColor
                }

                CustomTextInput {
                    id: apiUrlField
                    Layout.fillWidth: true
                    text: setting.get("server/baseUrl", "https://api.example.com")
                }

                RowLayout {
                    spacing: 10

                    CustomButton {
                        text: qsTr("保存配置")
                        backgroundColor: ThemeManager.successColor
                        hoverColor: Qt.darker(ThemeManager.successColor, 1.1)
                        pressedColor: Qt.darker(ThemeManager.successColor, 1.2)
                        enabled: apiUrlField.text.length > 0
                        onClicked: {
                            var url = apiUrlField.text.trim();
                            if (!url.startsWith("http://") && !url.startsWith("https://")) {
                                modalDialog.showInfo(qsTr("操作失败"), qsTr("请输入完整的URL地址（包含http://或https://）"));
                                return;
                            }

                            if (!setting.isHttpsUrl(url)) {
                                httpsWarningDialog.targetUrl = url;
                                httpsWarningDialog.open();
                                return;
                            }

                            setting.updateServerConfig(url);
                            modalDialog.showInfo(qsTr("操作成功"), qsTr("API服务器地址已成功保存！"));
                        }
                    }

                    CustomButton {
                        text: qsTr("重置为默认")
                        is2ndColor: true
                        onClicked: {
                            apiUrlField.text = "https://api.example.com";
                        }
                    }
                }
            }

            Divider {}

            Label {
                text: qsTr("关于")
                font.bold: true
                font.pixelSize: 16
                color: ThemeManager.textColor
                Layout.topMargin: 10
            }

            CustomButton {
                text: qsTr("GitHub主页")
                onClicked: Qt.openUrlExternally("https://github.com/sakurakugu/MyTodo")
                Layout.leftMargin: 20
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
            }
        }

        // 导出文件对话框
        FileDialog {
            id: exportFileDialog
            title: qsTr("选择导出位置")
            fileMode: FileDialog.SaveFile
            nameFilters: ["JSON files (*.json)", "All files (*)"]
            defaultSuffix: "json"
            selectedFile: {
                var now = new Date();
                var dateStr = now.getFullYear() + "-" + String(now.getMonth() + 1).padStart(2, '0') + "-" + String(now.getDate()).padStart(2, '0') + "_" + String(now.getHours()).padStart(2, '0') + "：" + String(now.getMinutes()).padStart(2, '0');
                return "file:///" + "MyTodo_导出_" + dateStr + ".json";
            }
            onAccepted: {
                var filePath = selectedFile.toString().replace("file:///", "");
                setting.exportToJsonFile(filePath);
            }
        }

        // 导入文件对话框
        FileDialog {
            id: importFileDialog
            title: qsTr("选择要导入的文件")
            fileMode: FileDialog.OpenFile
            nameFilters: ["JSON files (*.json)", "All files (*)"]
            onAccepted: {
                var filePath = selectedFile.toString().replace("file:///", "");

                // 使用新的自动解决方法：先导入无冲突项目，返回冲突项目列表
                var conflicts = todoManager.importTodosFromJson(filePath);

                if (conflicts.length > 0)
                // 有冲突，显示冲突处理对话框
                {} else {
                    // 没有冲突，所有项目已自动导入
                    modalDialog.showInfo(qsTr("导入成功"), qsTr("待办事项已成功导入！"));
                }
            }
        }

        // HTTPS警告对话框
        ModalDialog {
            id: httpsWarningDialog
            dialogTitle: qsTr("安全警告")
            message: qsTr("您输入的地址使用HTTP协议，这可能不安全。\n建议使用HTTPS协议以保护您的数据安全。\n\n是否仍要保存此配置？")
            property string targetUrl: ""

            onConfirmed: {
                setting.updateServerConfig(targetUrl);
                modalDialog.showInfo(qsTr("操作成功"), qsTr("API服务器地址已成功保存！"));
            }
        }

        // 清空配置确认对话框
        ModalDialog {
            id: clearConfigDialog
            dialogTitle: qsTr("清空所有配置")
            dialogWidth: 350
            message: qsTr("警告：此操作将清空所有配置设置！\n\n确定要继续吗？此操作无法撤销。")

            onConfirmed: {
                setting.clear();
                modalDialog.showInfo(qsTr("操作成功"), qsTr("所有配置已清空"));
            }
        }

        // 通用对话框（只有确认）
        ModalDialog {
            id: modalDialog
            isShowCancelButton: false

            onConfirmed: {
                modalDialog.close();
            }
        }
    }
}
