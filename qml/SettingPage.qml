import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs

Page {
    id: settingPage

    property var root
    property var stackView
    property bool preventDragging: setting.get("setting/preventDragging", false) // 是否允许拖动

    // 主题管理器
    ThemeManager {
        id: theme
    }

    // 应用代理设置函数
    function applyProxySettings() {
        if (!setting.getProxyEnabled()) {
            // 禁用代理
            networkRequest.setProxyConfig(0, "", 0, "", ""); // NoProxy
        } else {
            var proxyType = setting.getProxyType();
            var host = setting.getProxyHost();
            var port = setting.getProxyPort();
            var username = setting.getProxyUsername();
            var password = setting.getProxyPassword();

            networkRequest.setProxyConfig(proxyType, host, port, username, password);
        }
    }

    // 登录相关对话框组件
    LoginStatusDialogs {
        id: loginStatusDialogs
    }

    // 标题栏
    Rectangle {
        id: titleBar
        height: 30
        width: parent.width
        color: theme.backgroundColor

        // 窗口拖拽处理区域
        WindowDragHandler {
            anchors.fill: parent
            targetWindow: root
        }

        // 左侧返回按钮和标题
        RowLayout {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            height: 30
            spacing: 8

            IconButton {
                text: "\ue8fa"
                textColor: theme.textColor
                fontSize: 16
                onClicked: stackView.pop()
                isDarkMode: globalState.isDarkMode
            }

            Text {
                text: qsTr("设置")
                font.bold: true
                font.pixelSize: 16
                color: theme.textColor
            }
        }

        // 右侧窗口控制按钮
        RowLayout {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: 30
            spacing: 0

            // 最小化按钮
            IconButton {
                text: "\ue65a"
                onClicked: homePage.showMinimized()
                textColor: theme.textColor
                fontSize: 16
                isDarkMode: globalState.isDarkMode
            }

            // 最大化/恢复按钮
            IconButton {
                text: root.visibility === Window.Maximized ? "\ue600" : "\ue65b"
                onClicked: {
                    if (root.visibility === Window.Maximized) {
                        root.showNormal();
                    } else {
                        root.showMaximized();
                    }
                }
                textColor: theme.textColor
                fontSize: 16
                isDarkMode: globalState.isDarkMode
            }

            // 关闭按钮
            IconButton {
                text: "\ue8d1"
                onClicked: root.close()
                fontSize: 16
                textColor: theme.textColor
                isDarkMode: globalState.isDarkMode
            }
        }
    }

    ScrollView {
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width - 40  // 减去左右边距
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 15

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    id: userProfileContent
                    spacing: 10

                    // 显示用户头像
                    Rectangle {
                        width: 30
                        height: 30
                        radius: 15                               ///< 圆形头像
                        color: theme.secondaryBackgroundColor    ///< 使用主题次要背景色
                        Layout.alignment: Qt.AlignVCenter        ///< 垂直居中对齐

                        /// 头像图标
                        Text {
                            anchors.centerIn: parent
                            text: "👤"                      ///< 默认用户图标
                            font.pixelSize: 18
                        }
                    }

                    // 显示用户名
                    Text {
                        text: userAuth.username !== "" ? userAuth.username : qsTr("未登录")
                        color: theme.textColor      ///< 使用主题文本颜色
                        font.bold: true                     ///< 粗体字
                        font.pixelSize: 16                  ///< 字体大小
                        Layout.alignment: Qt.AlignVCenter   ///< 垂直居中对齐
                        horizontalAlignment: Text.AlignLeft ///< 水平左对齐
                    }
                }
            }

            Label {
                text: qsTr("外观设置")
                font.bold: true
                font.pixelSize: 16
                color: theme.textColor
            }

            Switch {
                id: darkModeCheckBox
                text: qsTr("深色模式")
                checked: globalState.isDarkMode
                enabled: !followSystemThemeCheckBox.checked

                property bool isInitialized: false

                Component.onCompleted: {
                    isInitialized = true;
                }

                onCheckedChanged: {
                    if (!isInitialized) {
                        return; // 避免初始化时触发
                    }
                    globalState.isDarkMode = checked;
                    // 保存设置到配置文件
                    setting.save("setting/isDarkMode", checked);
                }
            }

            Switch {
                id: followSystemThemeCheckBox
                text: qsTr("跟随系统深色模式")
                checked: setting.get("setting/followSystemTheme", false)

                property bool isInitialized: false

                Component.onCompleted: {
                    isInitialized = true;
                    if (checked) {
                        // 如果启用跟随系统，立即同步系统主题
                        var systemDarkMode = globalState.isSystemDarkMode;
                        if (globalState.isDarkMode !== systemDarkMode) {
                            globalState.isDarkMode = systemDarkMode;
                            setting.save("setting/isDarkMode", systemDarkMode);
                        }
                    }
                }

                onCheckedChanged: {
                    if (!isInitialized) {
                        return; // 避免初始化时触发
                    }

                    setting.save("setting/followSystemTheme", checked);

                    if (checked) {
                        // 启用跟随系统时，立即同步系统主题
                        var systemDarkMode = globalState.isSystemDarkMode;
                        if (globalState.isDarkMode !== systemDarkMode) {
                            globalState.isDarkMode = systemDarkMode;
                            setting.save("setting/isDarkMode", systemDarkMode);
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
                                globalState.isDarkMode = systemDarkMode;
                                setting.save("setting/isDarkMode", systemDarkMode);
                            }
                        }
                    }
                }
            }

            Switch {
                id: preventDraggingCheckBox
                text: qsTr("防止拖动窗口（小窗口模式）")
                checked: settingPage.preventDragging
                enabled: globalState.isDesktopWidget
                onCheckedChanged: {
                    settingPage.preventDragging = checked;
                    setting.save("setting/preventDragging", settingPage.preventDragging);
                }
            }

            Switch {
                id: autoStartSwitch
                text: qsTr("开机自启动")
                checked: globalState.isAutoStartEnabled()
                onCheckedChanged: {
                    globalState.setAutoStart(checked);
                }
            }

            Switch {
                id: autoSyncSwitch
                text: !todoManager.isLoggedIn ? qsTr("自动同步（未登录）") : qsTr("自动同步")
                checked: todoSyncServer.isAutoSyncEnabled

                property bool isInitialized: false

                Component.onCompleted: {
                    isInitialized = true;
                }

                onCheckedChanged: {
                    if (!isInitialized) {
                        return; // 避免初始化时触发
                    }

                    if (checked && !todoManager.isLoggedIn) {
                        // 如果要开启自动同步但未登录，显示提示并重置开关
                        autoSyncSwitch.checked = false;
                        loginStatusDialogs.showLoginRequired();
                    } else {
                        todoSyncServer.setAutoSyncEnabled(checked);
                    }
                }
            }

            // 在线状态显示（只读）
            Row {
                spacing: 10
                Label {
                    text: qsTr("连接状态:")
                    anchors.verticalCenter: parent.verticalCenter
                }
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: todoManager.isOnline ? "#4CAF50" : "#F44336"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Label {
                text: qsTr("网络代理设置")
                font.bold: true
                font.pixelSize: 16
                color: theme.textColor
            }

            Switch {
                id: proxyEnabledSwitch
                text: qsTr("启用代理")
                checked: setting.getProxyEnabled()

                property bool isInitialized: false

                Component.onCompleted: {
                    isInitialized = true;
                }

                onCheckedChanged: {
                    if (!isInitialized) {
                        return;
                    }
                    setting.setProxyEnabled(checked);
                    // 应用代理设置
                    applyProxySettings();
                }
            }

            ComboBox {
                id: proxyTypeCombo
                enabled: proxyEnabledSwitch.checked
                model: [qsTr("不使用代理"), qsTr("系统代理"), qsTr("HTTP代理"), qsTr("SOCKS5代理")]
                currentIndex: setting.getProxyType()

                property bool isInitialized: false

                Component.onCompleted: {
                    isInitialized = true;
                }

                onCurrentIndexChanged: {
                    if (!isInitialized) {
                        return;
                    }
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

                    TextField {
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

                    TextField {
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

                    TextField {
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
                Label {
                    text: qsTr("连接状态:")
                    anchors.verticalCenter: parent.verticalCenter
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

            Label {
                text: qsTr("数据管理")
                font.bold: true
                font.pixelSize: 16
                color: theme.textColor
                Layout.topMargin: 10
            }

            RowLayout {
                spacing: 10

                Button {
                    text: qsTr("导出待办事项")
                    background: Rectangle {
                        color: "#27ae60"
                        radius: 4
                    }
                    onClicked: exportFileDialog.open()
                }

                Button {
                    text: qsTr("导入待办事项")
                    background: Rectangle {
                        color: "#3498db"
                        radius: 4
                    }
                    onClicked: importFileDialog.open()
                }
            }

            Label {
                text: qsTr("关于")
                font.bold: true
                font.pixelSize: 16
                color: theme.textColor
                Layout.topMargin: 10
            }

            Button {
                text: qsTr("GitHub主页")
                background: Rectangle {
                    color: "#0f85d3"
                    radius: 4
                }
                onClicked: Qt.openUrlExternally("https://github.com/sakurakugu/MyTodo")
            }

            Label {
                text: qsTr("配置文件管理")
                font.bold: true
                font.pixelSize: 16
                color: theme.textColor
                Layout.topMargin: 10
            }

            ColumnLayout {
                spacing: 10
                Layout.fillWidth: true

                // 配置文件路径显示（仅对文件类型显示）
                ColumnLayout {
                    spacing: 5
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("配置文件路径:")
                        color: theme.textColor
                    }

                    TextField {
                        id: configPathField
                        Layout.fillWidth: true
                        text: setting.getConfigFilePath()
                        readOnly: true
                        color: theme.textColor
                        background: Rectangle {
                            color: theme.secondaryBackgroundColor
                            border.color: theme.borderColor
                            border.width: 1
                            radius: 4
                        }
                    }

                    RowLayout {
                        spacing: 10

                        Button {
                            text: qsTr("打开目录")
                            background: Rectangle {
                                color: "#27ae60"
                                radius: 4
                            }
                            onClicked: {
                                if (!setting.openConfigFilePath()) {
                                    openDirErrorDialog.open();
                                }
                            }
                        }

                        Button {
                            text: qsTr("清空配置")
                            background: Rectangle {
                                color: "#e74c3c"
                                radius: 4
                            }
                            onClicked: clearConfigDialog.open()
                        }
                    }
                }
            }

            Label {
                text: qsTr("服务器配置")
                font.bold: true
                font.pixelSize: 16
                color: theme.textColor
                Layout.topMargin: 10
            }

            ColumnLayout {
                spacing: 10
                Layout.fillWidth: true

                Label {
                    text: qsTr("API服务器地址:")
                    color: theme.textColor
                }

                TextField {
                    id: apiUrlField
                    Layout.fillWidth: true
                    // placeholderText: qsTr("请输入API服务器地址")
                    text: setting.get("server/baseUrl", "https://api.example.com")
                    color: theme.textColor
                    background: Rectangle {
                        color: theme.secondaryBackgroundColor
                        border.color: theme.borderColor
                        border.width: 1
                        radius: 4
                    }
                }

                RowLayout {
                    spacing: 10

                    Button {
                        text: qsTr("保存配置")
                        background: Rectangle {
                            color: "#27ae60"
                            radius: 4
                        }
                        enabled: apiUrlField.text.length > 0
                        onClicked: {
                            var url = apiUrlField.text.trim();
                            if (!url.startsWith("http://") && !url.startsWith("https://")) {
                                apiConfigErrorDialog.message = qsTr("请输入完整的URL地址（包含http://或https://）");
                                apiConfigErrorDialog.open();
                                return;
                            }

                            if (!setting.isHttpsUrl(url)) {
                                httpsWarningDialog.targetUrl = url;
                                httpsWarningDialog.open();
                                return;
                            }

                            setting.updateServerConfig(url);
                            apiConfigSuccessDialog.open();
                        }
                    }

                    Button {
                        text: qsTr("重置为默认")
                        background: Rectangle {
                            color: "#95a5a6"
                            radius: 4
                        }
                        onClicked: {
                            apiUrlField.text = "https://api.example.com";
                        }
                    }
                }
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
                var dateStr = now.getFullYear() + "-" + String(now.getMonth() + 1).padStart(2, '0') + "-" + String(now.getDate()).padStart(2, '0') + "_" + String(now.getHours()).padStart(2, '0') + "-" + String(now.getMinutes()).padStart(2, '0');
                return "file:///" + "MyTodo_导出_" + dateStr + ".json";
            }
            onAccepted: {
                var filePath = selectedFile.toString().replace("file:///", "");
                if (todoManager.exportTodos(filePath)) {
                    exportSuccessDialog.open();
                } else {
                    exportErrorDialog.open();
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
                    importSuccessDialog.open();
                }
            }
        }

        // 导出成功对话框
        BaseDialog {
            id: exportSuccessDialog
            dialogTitle: qsTr("导出成功")
            dialogWidth: 300
            dialogHeight: 150
            showStandardButtons: true

            Label {
                text: qsTr("待办事项已成功导出！")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
            }
        }

        // 导出失败对话框
        BaseDialog {
            id: exportErrorDialog
            dialogTitle: qsTr("导出失败")
            dialogWidth: 400
            dialogHeight: 180
            showStandardButtons: true

            Label {
                text: qsTr("导出待办事项时发生错误，请检查文件路径和权限。")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }
        }

        // 导入成功对话框
        BaseDialog {
            id: importSuccessDialog
            dialogTitle: qsTr("导入成功")
            dialogWidth: 300
            dialogHeight: 150
            showStandardButtons: true

            Label {
                text: qsTr("待办事项已成功导入！")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
            }
        }

        // 导入失败对话框
        BaseDialog {
            id: importErrorDialog
            dialogTitle: qsTr("导入失败")
            dialogWidth: 400
            dialogHeight: 180
            showStandardButtons: true

            Label {
                text: qsTr("导入待办事项时发生错误，请检查文件格式和内容。")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }
        }

        // 冲突处理对话框
        BaseDialog {
            id: conflictResolutionDialog
            dialogTitle: qsTr("检测到冲突")
            dialogWidth: Math.min(800, parent.width * 0.9)
            dialogHeight: Math.min(600, parent.height * 0.8)
            showStandardButtons: false

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
                color: theme.textColor
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
                    color: theme.textColor
                }

                Button {
                    text: qsTr("全部跳过")
                    font.pixelSize: 10
                    background: Rectangle {
                        color: parent.pressed ? (theme.isDarkMode ? "#34495e" : "#bdc3c7") : (parent.hovered ? (theme.isDarkMode ? "#2c3e50" : "#ecf0f1") : (theme.isDarkMode ? "#2c3e50" : "white"))
                        border.color: theme.isDarkMode ? "#34495e" : "#bdc3c7"
                        border.width: 1
                        radius: 4
                    }
                    onClicked: {
                        for (var i = 0; i < conflictResolutionDialog.conflicts.length; i++) {
                            conflictResolutionDialog.conflictResolutions[conflictResolutionDialog.conflicts[i].id] = "skip";
                        }
                        // 刷新界面
                        conflictResolutionDialog.close();
                        conflictResolutionDialog.open();
                    }
                }

                Button {
                    text: qsTr("全部覆盖")
                    font.pixelSize: 10
                    background: Rectangle {
                        color: parent.pressed ? (theme.isDarkMode ? "#34495e" : "#bdc3c7") : (parent.hovered ? (theme.isDarkMode ? "#2c3e50" : "#ecf0f1") : (theme.isDarkMode ? "#2c3e50" : "white"))
                        border.color: theme.isDarkMode ? "#34495e" : "#bdc3c7"
                        border.width: 1
                        radius: 4
                    }
                    onClicked: {
                        for (var i = 0; i < conflictResolutionDialog.conflicts.length; i++) {
                            conflictResolutionDialog.conflictResolutions[conflictResolutionDialog.conflicts[i].id] = "overwrite";
                        }
                        // 刷新界面
                        conflictResolutionDialog.close();
                        conflictResolutionDialog.open();
                    }
                }

                Button {
                    text: qsTr("全部智能合并")
                    font.pixelSize: 10
                    background: Rectangle {
                        color: parent.pressed ? (theme.isDarkMode ? "#34495e" : "#bdc3c7") : (parent.hovered ? (theme.isDarkMode ? "#2c3e50" : "#ecf0f1") : (theme.isDarkMode ? "#2c3e50" : "white"))
                        border.color: theme.isDarkMode ? "#34495e" : "#bdc3c7"
                        border.width: 1
                        radius: 4
                    }
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

                Button {
                    text: qsTr("取消")
                    background: Rectangle {
                        color: parent.pressed ? (theme.isDarkMode ? "#34495e" : "#bdc3c7") : (parent.hovered ? (theme.isDarkMode ? "#2c3e50" : "#ecf0f1") : (theme.isDarkMode ? "#2c3e50" : "white"))
                        border.color: theme.isDarkMode ? "#34495e" : "#bdc3c7"
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
                    onClicked: conflictResolutionDialog.reject()
                }

                Button {
                    text: qsTr("确定")
                    background: Rectangle {
                        color: parent.pressed ? "#2980b9" : (parent.hovered ? "#3498db" : "#3498db")
                        border.color: "#2980b9"
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
                    onClicked: {
                        if (todoManager.importTodosWithIndividualResolution(conflictResolutionDialog.selectedFilePath, conflictResolutionDialog.conflictResolutions)) {
                            conflictResolutionDialog.accept();
                            importSuccessDialog.open();
                        } else {
                            conflictResolutionDialog.reject();
                            importErrorDialog.open();
                        }
                    }
                }
            }
        }

        // API配置成功对话框
        Dialog {
            id: apiConfigSuccessDialog
            title: qsTr("配置成功")
            standardButtons: Dialog.Ok
            Label {
                text: qsTr("API服务器地址已成功保存！")
            }
        }

        // API配置错误对话框
        Dialog {
            id: apiConfigErrorDialog
            title: qsTr("配置错误")
            standardButtons: Dialog.Ok
            property string message: ""
            Label {
                text: apiConfigErrorDialog.message
            }
        }

        // HTTPS警告对话框
        Dialog {
            id: httpsWarningDialog
            title: qsTr("安全警告")
            standardButtons: Dialog.Yes | Dialog.No
            property string targetUrl: ""

            Label {
                text: qsTr("您输入的地址使用HTTP协议，这可能不安全。\n建议使用HTTPS协议以保护您的数据安全。\n\n是否仍要保存此配置？")
                wrapMode: Text.WordWrap
            }

            onAccepted: {
                setting.updateServerConfig(targetUrl);
                apiConfigSuccessDialog.open();
            }
        }

        // 清空配置确认对话框
        BaseDialog {
            id: clearConfigDialog
            dialogTitle: qsTr("清空所有配置")
            dialogWidth: 350
            dialogHeight: 200
            showStandardButtons: true
            standardButtons: Dialog.Yes | Dialog.No

            Label {
                text: qsTr("警告：此操作将清空所有配置设置！\n\n确定要继续吗？此操作无法撤销。")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                color: "#e74c3c"
                font.pixelSize: 14
            }

            onAccepted: {
                setting.clear();
                clearConfigSuccessDialog.open();
            }
        }

        // 成功/错误提示对话框
        BaseDialog {
            id: storageChangeSuccessDialog
            dialogTitle: qsTr("操作成功")
            dialogWidth: 300
            dialogHeight: 150
            showStandardButtons: true

            Label {
                text: qsTr("存储类型已成功更改！")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
            }
        }

        BaseDialog {
            id: storageChangeErrorDialog
            dialogTitle: qsTr("操作失败")
            dialogWidth: 300
            dialogHeight: 150
            showStandardButtons: true

            Label {
                text: qsTr("更改存储类型时发生错误，请重试。")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
            }
        }

        BaseDialog {
            id: pathChangeSuccessDialog
            dialogTitle: qsTr("操作成功")
            dialogWidth: 300
            dialogHeight: 150
            showStandardButtons: true

            Label {
                text: qsTr("配置文件路径已成功更改！")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
            }
        }

        BaseDialog {
            id: pathChangeErrorDialog
            dialogTitle: qsTr("操作失败")
            dialogWidth: 350
            dialogHeight: 150
            showStandardButtons: true

            Label {
                text: qsTr("更改配置文件路径时发生错误，请检查路径是否有效。")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
            }
        }

        BaseDialog {
            id: pathResetSuccessDialog
            dialogTitle: qsTr("操作成功")
            dialogWidth: 350
            dialogHeight: 150
            showStandardButtons: true

            Label {
                text: qsTr("配置文件路径已重置为选定的默认位置！")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
            }
        }

        BaseDialog {
            id: pathResetErrorDialog
            dialogTitle: qsTr("操作失败")
            dialogWidth: 300
            dialogHeight: 150
            showStandardButtons: true

            Label {
                text: qsTr("重置配置文件路径时发生错误。")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
            }
        }

        BaseDialog {
            id: clearConfigSuccessDialog
            dialogTitle: qsTr("操作成功")
            dialogWidth: 300
            dialogHeight: 150
            showStandardButtons: true

            Label {
                text: qsTr("所有配置已清空！")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
            }
        }

        BaseDialog {
            id: openDirErrorDialog
            dialogTitle: qsTr("操作失败")
            dialogWidth: 350
            dialogHeight: 150
            showStandardButtons: true

            Label {
                text: qsTr("无法打开配置文件目录。")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor
                font.pixelSize: 14
            }
        }
    }
}
