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

    ColumnLayout {
        anchors.fill: parent
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
                todoModel.isOnline = checked;
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
            if (todoModel.importTodos(filePath)) {
                importSuccessDialog.open();
            } else {
                importErrorDialog.open();
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
}
