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

    property bool preventDragging: setting.get("setting/preventDragging", false) // 是否允许拖动

    // 应用代理设置函数
    function applyProxySettings() {
        if (!setting.getProxyEnabled()) {
            // 禁用代理
            setting.setProxyConfig(0, "", 0, "", "", false); // NoProxy
        } else {
            var proxyType = setting.getProxyType();
            var host = setting.getProxyHost();
            var port = setting.getProxyPort();
            var username = setting.getProxyUsername();
            var password = setting.getProxyPassword();

            setting.setProxyConfig(proxyType, host, port, username, password, true);
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

            SwitchRow {
                id: darkModeCheckBox
                text: qsTr("深色模式")
                checked: globalState.isDarkMode
                enabled: !followSystemThemeCheckBox.checked
                leftMargin: 10

                onCheckedChanged: {
                    globalState.isDarkMode = checked;
                    // 保存设置到配置文件
                    setting.save("setting/isDarkMode", checked);
                }
            }

            SwitchRow {
                id: followSystemThemeCheckBox
                text: qsTr("跟随系统深色模式")
                checked: setting.get("setting/followSystemTheme", false)
                leftMargin: 10

                Component.onCompleted: {
                    if (checked) {
                        // 如果启用跟随系统，立即同步系统主题
                        var systemDarkMode = globalState.isSystemDarkMode;
                        if (globalState.isDarkMode !== systemDarkMode) {
                            darkModeCheckBox.checked = systemDarkMode;
                        }
                    }
                }

                onCheckedChanged: {
                    setting.save("setting/followSystemTheme", checked);

                    if (checked) {
                        // 启用跟随系统时，立即同步系统主题
                        var systemDarkMode = globalState.isSystemDarkMode;
                        if (globalState.isDarkMode !== systemDarkMode) {
                            darkModeCheckBox.checked = systemDarkMode;
                        }
                    }
                }

                // 监听系统主题变化
                Connections {
                    target: globalState
                    function onSystemDarkModeChanged() {
                        if (followSystemThemeCheckBox.checked) {
                            var systemDarkMode = globalState.isSystemDarkMode;
                            if (globalState.isDarkMode !== systemDarkMode) {
                                darkModeCheckBox.checked = systemDarkMode;
                            }
                        }
                    }
                }
            }

            SwitchRow {
                id: preventDraggingCheckBox
                text: qsTr("防止拖动窗口（小窗口模式）")
                checked: settingPage.preventDragging
                enabled: globalState.isDesktopWidget
                leftMargin: 10
                onCheckedChanged: {
                    settingPage.preventDragging = checked;
                    setting.save("setting/preventDragging", settingPage.preventDragging);
                }
            }

            SwitchRow {
                id: autoStartSwitch
                text: qsTr("开机自启动")
                checked: globalState.isAutoStartEnabled()
                leftMargin: 10
                onCheckedChanged: {
                    globalState.setAutoStart(checked);
                }
            }

            SwitchRow {
                text: todoManager.isLoggedIn ? qsTr("自动同步") : qsTr("自动同步（未登录）")
                checked: todoSyncServer.isAutoSyncEnabled
                leftMargin: 10

                onCheckedChanged: {
                    if (checked) {
                        // 如果未登录，显示提示并重置开关
                        if (!todoManager.isLoggedIn) {
                            toggle();
                            settingPage.loginStatusDialogs.showLoginRequired();
                        } else {
                            todoSyncServer.setAutoSyncEnabled(checked);
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

            SwitchRow {
                id: proxyEnabledSwitch
                text: qsTr("启用代理")
                checked: setting.getProxyEnabled()
                leftMargin: 10

                onCheckedChanged: {
                    setting.setProxyEnabled(checked);
                    // 应用代理设置
                    applyProxySettings();
                }
            }

            ComboBox {
                id: proxyTypeCombo
                enabled: proxyEnabledSwitch.checked
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

                    SpinBox {
                        id: proxyPortSpinBox
                        from: 1
                        to: 65535
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

            // 在线状态显示（只读）
            Row {
                spacing: 10
                Layout.leftMargin: 20
                Label {
                    text: qsTr("连接状态:")
                    anchors.verticalCenter: parent.verticalCenter
                    color: ThemeManager.textColor
                }
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: todoManager.isOnline ? "#4CAF50" : "#F44336"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Label {
                    text: todoManager.isOnline ? qsTr("在线") : qsTr("离线")
                    color: todoManager.isOnline ? "#4CAF50" : "#F44336"
                    anchors.verticalCenter: parent.verticalCenter
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
                var dateStr = now.getFullYear() + "-" + String(now.getMonth() + 1).padStart(2, '0') + "-" + String(now.getDate()).padStart(2, '0') + "_" + String(now.getHours()).padStart(2, '0') + ":" + String(now.getMinutes()).padStart(2, '0');
                return "file:///" + "MyTodo_导出_" + dateStr + ".json";
            }
            onAccepted: {
                var filePath = selectedFile.toString().replace("file:///", "");
                if (todoManager.exportTodos(filePath)) {
                    modalDialog.showInfo(qsTr("导出成功"), qsTr("待办事项已成功导出！"));
                } else {
                    modalDialog.showError(qsTr("导出失败"), qsTr("导出待办事项时发生错误，请检查文件路径和权限。"));
                }
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
                var conflicts = todoManager.importTodosWithAutoResolution(filePath);

                if (conflicts.length > 0) {
                    // 有冲突，显示冲突处理对话框
                    conflictResolutionDialog.conflicts = conflicts;
                    conflictResolutionDialog.selectedFilePath = filePath;
                    conflictResolutionDialog.open();
                } else {
                    // 没有冲突，所有项目已自动导入
                    modalDialog.showInfo(qsTr("导入成功"), qsTr("待办事项已成功导入！"));
                }
            }
        }

        // 冲突处理对话框
        BaseDialog {
            id: conflictResolutionDialog
            dialogTitle: qsTr("检测到冲突")
            dialogWidth: Math.min(800, parent.width * 0.9)
            dialogHeight: Math.min(600, parent.height * 0.8)

            property var conflicts: []
            property string selectedFilePath: ""
            property var conflictResolutions: ({})

            onOpened: {
                // 初始化每个冲突项的处理方式为"skip"
                conflictResolutions = {};
                for (var i = 0; i < conflicts.length; i++) {
                    conflictResolutions[conflicts[i].id] = "skip";
                }
            }

            Label {
                text: qsTr("导入文件中发现 %1 个待办事项与现有数据存在ID冲突，请为每个冲突项选择处理方式：").arg(conflictResolutionDialog.conflicts.length)
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                font.bold: true
                color: ThemeManager.textColor
                font.pixelSize: 14
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 400
                clip: true

                Column {
                    width: parent.width
                    spacing: 10

                    Repeater {
                        model: conflictResolutionDialog.conflicts

                        delegate: Rectangle {
                            width: parent.width
                            height: 350
                            color: index % 2 === 0 ? "#f8f9fa" : "#ffffff"
                            border.color: "#dee2e6"
                            border.width: 1
                            radius: 5

                            Column {
                                id: conflictItemColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 15
                                spacing: 10

                                Label {
                                    text: qsTr("冲突项目 %1 (ID: %2)").arg(index + 1).arg(modelData.id)
                                    font.bold: true
                                    font.pixelSize: 14
                                    color: "#dc3545"
                                }

                                Row {
                                    width: parent.width
                                    spacing: 20

                                    // 现有数据
                                    Column {
                                        width: (parent.width - 40) / 2

                                        Rectangle {
                                            width: parent.width
                                            height: 2
                                            color: "#28a745"
                                        }

                                        Label {
                                            text: qsTr("现有数据")
                                            font.bold: true
                                            font.pixelSize: 12
                                            color: "#28a745"
                                        }

                                        Label {
                                            text: qsTr("标题: %1").arg(modelData.existingTitle || "")
                                            wrapMode: Text.WordWrap
                                            width: parent.width
                                            font.pixelSize: 11
                                        }

                                        Label {
                                            text: qsTr("描述: %1").arg(modelData.existingDescription || "")
                                            wrapMode: Text.WordWrap
                                            width: parent.width
                                            font.pixelSize: 11
                                            maximumLineCount: 2
                                            elide: Text.ElideRight
                                        }

                                        Row {
                                            width: parent.width
                                            spacing: 10

                                            Label {
                                                text: qsTr("分类: %1").arg(modelData.existingCategory || "")
                                                font.pixelSize: 10
                                            }

                                            Label {
                                                text: qsTr("状态: %1").arg(modelData.existingStatus || "")
                                                font.pixelSize: 10
                                            }
                                        }

                                        Label {
                                            text: qsTr("更新时间: %1").arg(modelData.existingUpdatedAt ? modelData.existingUpdatedAt.toLocaleString() : "")
                                            font.pixelSize: 9
                                            color: "#6c757d"
                                        }
                                    }

                                    // 分隔线
                                    Rectangle {
                                        width: 2
                                        height: 200
                                        color: "#dee2e6"
                                    }

                                    // 导入数据
                                    Column {
                                        width: (parent.width - 40) / 2

                                        Rectangle {
                                            width: parent.width
                                            height: 2
                                            color: "#007bff"
                                        }

                                        Label {
                                            text: qsTr("导入数据")
                                            font.bold: true
                                            font.pixelSize: 12
                                            color: "#007bff"
                                        }

                                        Label {
                                            text: qsTr("标题: %1").arg(modelData.importTitle || "")
                                            wrapMode: Text.WordWrap
                                            width: parent.width
                                            font.pixelSize: 11
                                        }

                                        Label {
                                            text: qsTr("描述: %1").arg(modelData.importDescription || "")
                                            wrapMode: Text.WordWrap
                                            width: parent.width
                                            font.pixelSize: 11
                                            maximumLineCount: 2
                                            elide: Text.ElideRight
                                        }

                                        Row {
                                            width: parent.width
                                            spacing: 10

                                            Label {
                                                text: qsTr("分类: %1").arg(modelData.importCategory || "")
                                                font.pixelSize: 10
                                            }

                                            Label {
                                                text: qsTr("状态: %1").arg(modelData.importStatus || "")
                                                font.pixelSize: 10
                                            }
                                        }

                                        Label {
                                            text: qsTr("更新时间: %1").arg(modelData.importUpdatedAt ? modelData.importUpdatedAt.toLocaleString() : "")
                                            font.pixelSize: 9
                                            color: "#6c757d"
                                        }
                                    }
                                }

                                // 处理方式选择
                                Column {
                                    width: parent.width
                                    spacing: 5

                                    Label {
                                        text: qsTr("处理方式:")
                                        font.bold: true
                                        font.pixelSize: 12
                                    }

                                    Row {
                                        width: parent.width
                                        spacing: 15

                                        RadioButton {
                                            id: skipRadio
                                            text: qsTr("跳过")
                                            checked: true
                                            font.pixelSize: 11
                                            onCheckedChanged: {
                                                if (checked) {
                                                    conflictResolutionDialog.conflictResolutions[modelData.id] = "skip";
                                                }
                                            }
                                        }

                                        RadioButton {
                                            id: overwriteRadio
                                            text: qsTr("覆盖")
                                            font.pixelSize: 11
                                            onCheckedChanged: {
                                                if (checked) {
                                                    conflictResolutionDialog.conflictResolutions[modelData.id] = "overwrite";
                                                }
                                            }
                                        }

                                        RadioButton {
                                            id: mergeRadio
                                            text: qsTr("智能合并")
                                            font.pixelSize: 11
                                            onCheckedChanged: {
                                                if (checked) {
                                                    conflictResolutionDialog.conflictResolutions[modelData.id] = "merge";
                                                }
                                            }
                                        }
                                    }

                                    Label {
                                        text: {
                                            if (skipRadio.checked)
                                                return qsTr("保留现有数据，不导入此项目");
                                            if (overwriteRadio.checked)
                                                return qsTr("用导入的数据完全替换现有数据");
                                            if (mergeRadio.checked)
                                                return qsTr("保留更新时间较新的版本");
                                            return "";
                                        }
                                        font.pixelSize: 10
                                        color: "#6c757d"
                                        wrapMode: Text.WordWrap
                                        width: parent.width
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 批量操作按钮
            Row {
                Layout.fillWidth: true
                spacing: 10
                Layout.topMargin: 10

                Label {
                    text: qsTr("批量操作:")
                    font.bold: true
                    color: ThemeManager.textColor
                }

                CustomButton {
                    text: qsTr("全部跳过")
                    font.pixelSize: 10
                    is2ndColor: true
                    onClicked: {
                        for (var i = 0; i < conflictResolutionDialog.conflicts.length; i++) {
                            conflictResolutionDialog.conflictResolutions[conflictResolutionDialog.conflicts[i].id] = "skip";
                        }
                        // 刷新界面
                        conflictResolutionDialog.close();
                        conflictResolutionDialog.open();
                    }
                }

                CustomButton {
                    text: qsTr("全部覆盖")
                    font.pixelSize: 10
                    is2ndColor: true
                    onClicked: {
                        for (var i = 0; i < conflictResolutionDialog.conflicts.length; i++) {
                            conflictResolutionDialog.conflictResolutions[conflictResolutionDialog.conflicts[i].id] = "overwrite";
                        }
                        // 刷新界面
                        conflictResolutionDialog.close();
                        conflictResolutionDialog.open();
                    }
                }

                CustomButton {
                    text: qsTr("全部智能合并")
                    font.pixelSize: 10
                    is2ndColor: true
                    onClicked: {
                        for (var i = 0; i < conflictResolutionDialog.conflicts.length; i++) {
                            conflictResolutionDialog.conflictResolutions[conflictResolutionDialog.conflicts[i].id] = "merge";
                        }
                        // 刷新界面
                        conflictResolutionDialog.close();
                        conflictResolutionDialog.open();
                    }
                }
            }

            Row {
                Layout.alignment: Qt.AlignRight
                spacing: 10
                Layout.topMargin: 10

                CustomButton {
                    text: qsTr("取消")
                    is2ndColor: true
                    onClicked: conflictResolutionDialog.reject()
                }

                CustomButton {
                    text: qsTr("确定")
                    onClicked: {
                        if (todoManager.importTodosWithIndividualResolution(conflictResolutionDialog.selectedFilePath, conflictResolutionDialog.conflictResolutions)) {
                            conflictResolutionDialog.accept();
                            modalDialog.showInfo(qsTr("导入成功"), qsTr("待办事项已成功导入！"));
                        } else {
                            conflictResolutionDialog.reject();
                            modalDialog.showError(qsTr("导入失败"), qsTr("导入待办事项时发生错误，请检查文件格式和内容。"));
                        }
                    }
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
        }
    }
}
