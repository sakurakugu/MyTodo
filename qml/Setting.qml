import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs

Page {
    id: settingPage

    // 从父栈传入的属性
    property bool isDarkMode: false
    property bool preventDragging: settings.get("preventDragging", false)
    // 根窗口引用，用于更新全局属性
    property var rootWindow: null

    background: Rectangle {
        color: isDarkMode ? "#1e272e" : "white"
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "<-"
                onClicked: settingPage.StackView.view ? settingPage.StackView.view.pop() : null
            }
            Label {
                text: qsTr("设置")
                font.bold: true
                font.pixelSize: 16
                color: isDarkMode ? "#ecf0f1" : "black"
                Layout.leftMargin: 10
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        
        ColumnLayout {
            width: parent.width
            anchors.margins: 20
            spacing: 15

        Label {
            text: qsTr("外观设置")
            font.bold: true
            font.pixelSize: 16
            color: isDarkMode ? "#ecf0f1" : "black"
        }

        Switch {
            id: darkModeCheckBox
            text: qsTr("深色模式")
            checked: isDarkMode
            onCheckedChanged: {
                isDarkMode = checked;
                settings.save("isDarkMode", isDarkMode);
                if (rootWindow) rootWindow.isDarkMode = isDarkMode;
            }
        }

        Switch {
            id: preventDraggingCheckBox
            text: qsTr("防止拖动窗口（小窗口模式）")
            checked: preventDragging
            enabled: mainWindow.isDesktopWidget
            onCheckedChanged: {
                preventDragging = checked;
                settings.save("preventDragging", preventDragging);
                if (rootWindow) rootWindow.preventDragging = preventDragging;
            }
        }

        Switch {
            id: autoStartSwitch
            text: qsTr("开机自启动")
            checked: mainWindow.isAutoStartEnabled()
            onCheckedChanged: {
                mainWindow.setAutoStart(checked);
            }
        }

        Switch {
            id: autoSyncSwitch
            text: qsTr("自动同步")
            checked: todoModel.isOnline
            onCheckedChanged: {
                if (checked && !todoModel.isLoggedIn) {
                    // 如果要开启自动同步但未登录，显示提示并重置开关
                    autoSyncSwitch.checked = false;
                    loginRequiredDialog.open();
                } else {
                    todoModel.isOnline = checked;
                }
            }
        }

        Label {
            text: qsTr("数据管理")
            font.bold: true
            font.pixelSize: 16
            color: isDarkMode ? "#ecf0f1" : "black"
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
            color: isDarkMode ? "#ecf0f1" : "black"
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
            text: qsTr("服务器配置")
            font.bold: true
            font.pixelSize: 16
            color: isDarkMode ? "#ecf0f1" : "black"
            Layout.topMargin: 10
        }

        ColumnLayout {
            spacing: 10
            Layout.fillWidth: true

            Label {
                text: qsTr("API服务器地址:")
                color: isDarkMode ? "#ecf0f1" : "black"
            }

            TextField {
                id: apiUrlField
                Layout.fillWidth: true
                placeholderText: qsTr("请输入API服务器地址")
                text: settings.get("server/baseUrl", "https://api.example.com")
                color: isDarkMode ? "#ecf0f1" : "black"
                background: Rectangle {
                    color: isDarkMode ? "#2d3436" : "#f5f5f5"
                    border.color: isDarkMode ? "#34495e" : "#cccccc"
                    border.width: 1
                    radius: 4
                }
            }

            RowLayout {
                spacing: 10

                Button {
                    text: qsTr("保存配置")
                    enabled: apiUrlField.text.length > 0
                    onClicked: {
                        var url = apiUrlField.text.trim()
                        if (!url.startsWith("http://") && !url.startsWith("https://")) {
                            apiConfigErrorDialog.message = qsTr("请输入完整的URL地址（包含http://或https://）")
                            apiConfigErrorDialog.open()
                            return
                        }
                        
                        if (!todoModel.isHttpsUrl(url)) {
                            httpsWarningDialog.targetUrl = url
                            httpsWarningDialog.open()
                            return
                        }
                        
                        todoModel.updateServerConfig(url)
                        apiConfigSuccessDialog.open()
                    }
                    background: Rectangle {
                        color: "#27ae60"
                        radius: 4
                    }
                }

                Button {
                    text: qsTr("重置为默认")
                    onClicked: {
                        apiUrlField.text = "https://api.example.com"
                    }
                    background: Rectangle {
                        color: "#95a5a6"
                        radius: 4
                    }
                }
            }
        }

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
            var dateStr = now.getFullYear() + "-" + 
                         String(now.getMonth() + 1).padStart(2, '0') + "-" + 
                         String(now.getDate()).padStart(2, '0') + "_" +
                         String(now.getHours()).padStart(2, '0') + "-" +
                         String(now.getMinutes()).padStart(2, '0');
            return "file:///" + "MyTodo_导出_" + dateStr + ".json";
        }
        onAccepted: {
            var filePath = selectedFile.toString().replace("file:///", "");
            if (todoModel.exportTodos(filePath)) {
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
            var conflicts = todoModel.importTodosWithAutoResolution(filePath);
            
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
    Dialog {
        id: exportSuccessDialog
        title: qsTr("导出成功")
        standardButtons: Dialog.Ok
        Label {
            text: qsTr("待办事项已成功导出！")
        }
    }
    
    // 导出失败对话框
    Dialog {
        id: exportErrorDialog
        title: qsTr("导出失败")
        standardButtons: Dialog.Ok
        Label {
            text: qsTr("导出待办事项时发生错误，请检查文件路径和权限。")
        }
    }
    
    // 导入成功对话框
    Dialog {
        id: importSuccessDialog
        title: qsTr("导入成功")
        standardButtons: Dialog.Ok
        Label {
            text: qsTr("待办事项已成功导入！")
        }
    }
    
    // 导入失败对话框
    Dialog {
        id: importErrorDialog
        title: qsTr("导入失败")
        standardButtons: Dialog.Ok
        Label {
            text: qsTr("导入待办事项时发生错误，请检查文件格式和内容。")
        }
    }
    
    // 冲突处理对话框
    Dialog {
        id: conflictResolutionDialog
        title: qsTr("检测到冲突")
        modal: true
        width: Math.min(800, parent.width * 0.9)
        height: Math.min(600, parent.height * 0.8)
        anchors.centerIn: parent
        standardButtons: Dialog.NoButton
        
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
        
        contentItem: Column {
            spacing: 15
            anchors.fill: parent
            anchors.margins: 10
            
            Label {
                text: qsTr("导入文件中发现 %1 个待办事项与现有数据存在ID冲突，请为每个冲突项选择处理方式：").arg(conflictResolutionDialog.conflicts.length)
                wrapMode: Text.WordWrap
                width: parent.width
                font.bold: true
            }
            
            ScrollView {
                width: parent.width
                height: parent.height - 100
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
                                            if (skipRadio.checked) return qsTr("保留现有数据，不导入此项目");
                                            if (overwriteRadio.checked) return qsTr("用导入的数据完全替换现有数据");
                                            if (mergeRadio.checked) return qsTr("保留更新时间较新的版本");
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
                width: parent.width
                spacing: 10
                
                Label {
                    text: qsTr("批量操作:")
                    font.bold: true
                }
                
                Button {
                    text: qsTr("全部跳过")
                    font.pixelSize: 10
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
                anchors.right: parent.right
                spacing: 10
                
                Button {
                    text: qsTr("取消")
                    onClicked: conflictResolutionDialog.reject()
                }
                
                Button {
                    text: qsTr("确定")
                    highlighted: true
                    onClicked: {
                        if (todoModel.importTodosWithIndividualResolution(conflictResolutionDialog.selectedFilePath, conflictResolutionDialog.conflictResolutions)) {
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
            todoModel.updateServerConfig(targetUrl)
            apiConfigSuccessDialog.open()
        }
    }

    // 登录提示对话框
    Dialog {
        id: loginRequiredDialog
        title: qsTr("需要登录")
        modal: true
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: 300
        height: 150
        standardButtons: Dialog.Ok
        
        background: Rectangle {
            color: isDarkMode ? "#2c3e50" : "white"
            border.color: isDarkMode ? "#34495e" : "#bdc3c7"
            border.width: 1
            radius: 8
        }
        
        Label {
            text: qsTr("开启自动同步功能需要先登录账户。\n请先登录后再开启自动同步。")
            wrapMode: Text.WordWrap
            color: isDarkMode ? "#ecf0f1" : "black"
            anchors.centerIn: parent
        }
    }
}
