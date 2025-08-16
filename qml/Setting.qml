import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

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
}
