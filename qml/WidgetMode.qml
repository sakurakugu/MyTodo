/**
 * @file WidgetMode.qml
 * @brief 小组件模式组件
 *
 * 该文件定义了应用程序的小组件模式组件，用于在桌面小组件中显示和管理待办事项。
 * 它包含侧边栏、任务列表和任务详情等功能，用户可以在小组件中查看和管理待办事项。
 *
 * @author Sakurakugu
 * @date 2025-08-17 03:57:06(UTC+8) 周日
 * @version 2025-08-21 23:33:39(UTC+8) 周四
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Item {
    id: widgetMode

    // 从父组件传入的属性
    property bool isDesktopWidget: false
    property bool isShowAddTask: globalState.isShowAddTask
    property bool isShowSetting: globalState.isShowSetting
    property bool isShowTodos: false
    property bool isShowDropdown: globalState.isShowDropdown
    property bool isDarkMode: false
    property bool preventDragging: false

    // 自定义信号定义
    signal preventDraggingToggled(bool value)
    signal darkModeToggled(bool value)

    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: widgetMode.isDarkMode
    }

    // 小组件模式的设置弹出窗口
    Popup {
        id: settingsPopup
        y: 50
        width: 400
        height: 250
        padding: 0 // 消除Popup和contentItem之间的间隙

        modal: false // 非模态，允许同时打开多个弹出窗口
        focus: true
        closePolicy: Popup.NoAutoClose
        visible: isDesktopWidget && isShowSetting

        contentItem: Rectangle {
            color: theme.secondaryBackgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 5

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Label {
                    text: "设置"
                    font.bold: true
                    font.pixelSize: 16
                    color: theme.textColor
                }

                // 设置内容
                Switch {
                    id: darkModeCheckBox
                    text: "深色模式"
                    checked: isDarkMode

                    property bool isInitialized: false

                    Component.onCompleted: {
                        isInitialized = true;
                    }

                    onCheckedChanged: {
                        if (!isInitialized) {
                            return; // 避免初始化时触发
                        }
                        // 保存设置到配置文件
                        setting.save("setting/isDarkMode", checked);
                        // 发出信号通知父组件
                        widgetMode.darkModeToggled(checked);
                    }
                }

                Switch {
                    id: preventDraggingCheckBox
                    text: "防止拖动窗口（小窗口模式）"
                    checked: preventDragging
                    enabled: isDesktopWidget
                    onCheckedChanged: {
                        // 保存设置到配置文件
                        setting.save("setting/preventDragging", checked);
                        // 发出信号通知父组件
                        widgetMode.preventDraggingToggled(checked);
                    }
                }

                Switch {
                    id: autoSyncSwitch
                    text: "自动同步"
                    checked: setting.get("setting/autoSync", false)

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
                            setting.save("setting/autoSync", checked);
                        }
                    }
                }
            }
        }
    }

    // 小组件模式的添加任务弹出窗口
    Popup {
        id: addTaskPopup
        y: settingsPopup.visible ? settingsPopup.y + 250 + 6 : 50
        width: 400
        height: 250
        modal: false // 非模态，允许同时打开多个弹出窗口
        focus: true
        closePolicy: Popup.NoAutoClose
        visible: isDesktopWidget && isShowAddTask

        contentItem: Rectangle {
            color: theme.secondaryBackgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 5

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Label {
                    text: "添加任务"
                    font.bold: true
                    font.pixelSize: 16
                    color: theme.textColor
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TaskForm {
                        id: addTaskForm
                        width: parent.width
                        theme: widgetMode.theme
                        isCompactMode: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight

                    CustomButton {
                        text: "添加"
                        textColor: "white"
                        backgroundColor: theme.primaryColor
                        isDarkMode: widgetMode.isDarkMode
                        onClicked: {
                            if (addTaskForm.isValid()) {
                                var todoData = addTaskForm.getTodoData();
                                todoManager.addTodo(todoData.title, todoData.description, todoData.category, todoData.important);
                                addTaskForm.clear();
                                globalState.isShowAddTask = false;
                            }
                        }
                    }
                }
            }
        }
    }

    // 小组件模式的主内容区弹出窗口
    Popup {
        id: mainContentPopup

        property int baseHeight: 200
        property int calculatedY: {
            var baseY = 50;
            if (addTaskPopup.visible) {
                baseY = addTaskPopup.y + 250 + 6; // 使用固定高度避免循环依赖
            } else if (settingsPopup.visible) {
                baseY = settingsPopup.y + 250 + 6; // 使用固定高度避免循环依赖
            }
            return baseY;
        }

        // 根据其他弹出窗口的可见性动态计算位置
        y: calculatedY

        width: 400
        height: baseHeight
        modal: false // 非模态，允许同时打开多个弹出窗口
        focus: true
        closePolicy: Popup.NoAutoClose
        visible: isDesktopWidget && isShowTodos // 在小组件模式下且需要显示所有任务时显示

        contentItem: Rectangle {
            color: theme.secondaryBackgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 5

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Label {
                    text: "待办任务"
                    font.bold: true
                    font.pixelSize: 16
                    color: theme.textColor
                }

                // 待办列表
                ListView {
                    id: todoListPopupView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: todoManager

                    // 下拉刷新相关属性与逻辑（在小组件模式的弹窗中）
                    property bool refreshing: false
                    property int pullThreshold: 50
                    property real pullDistance: 0

                    onContentYChanged: {
                        pullDistance = contentY < 0 ? -contentY : 0;
                    }
                    onMovementEnded: {
                        if (contentY < -pullThreshold && atYBeginning && !refreshing) {
                            refreshing = true;
                            todoManager.syncWithServer();
                        }
                    }

                    header: Item {
                        width: todoListPopupView.width
                        height: todoListPopupView.refreshing ? 45 : Math.min(45, todoListPopupView.pullDistance)
                        visible: height > 0 || todoListPopupView.refreshing
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            BusyIndicator {
                                running: todoListPopupView.refreshing
                                visible: todoListPopupView.refreshing || todoListPopupView.pullDistance > 0
                                width: 18
                                height: 18
                            }
                            Label {
                                text: todoListPopupView.refreshing ? qsTr("正在同步...") : (todoListPopupView.pullDistance >= todoListPopupView.pullThreshold ? qsTr("释放刷新") : qsTr("下拉刷新"))
                                color: theme.textColor
                                font.pixelSize: 11
                            }
                        }
                    }

                    Connections {
                        target: todoManager
                        function onSyncStarted() {
                            if (!todoListPopupView.refreshing && todoListPopupView.atYBeginning) {
                                todoListPopupView.refreshing = true;
                            }
                        }
                        function onSyncCompleted(success, errorMessage) {
                            todoListPopupView.refreshing = false;
                            todoListPopupView.contentY = 0;
                        }
                    }

                    delegate: Rectangle {
                        width: todoListPopupView.width
                        height: 40
                        color: index % 2 === 0 ? theme.secondaryBackgroundColor : theme.backgroundColor

                        // 点击项目显示下拉窗口
                        MouseArea {
                            anchors.fill: parent
                            z: 0
                            onClicked: {
                                todoItemDropdown.currentTodoIndex = index;
                                todoItemDropdown.currentTodoData = {
                                    title: model.title,
                                    description: model.description,
                                    category: model.category,
                                    important: model.important
                                };
                                globalState.toggleDropdownVisible();
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 5

                            // 待办状态指示器
                            Rectangle {
                                width: 16
                                height: 16
                                radius: 8
                                color: model.status === "done" ? theme.completedColor : model.important === "高" ? theme.highImportantColor : model.important === "中" ? theme.mediumImportantColor : theme.lowImportantColor

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        todoManager.markAsDone(index);
                                        mouse.accepted = true;  // 阻止事件传播
                                    }
                                }
                            }

                            // 待办标题
                            Label {
                                text: model.title
                                color: theme.textColor
                                Layout.fillWidth: true
                            }

                            // 删除按钮
                            Rectangle {
                                width: 30
                                height: 30
                                color: "transparent"
                                border.width: 0
                                Text {
                                    anchors.centerIn: parent
                                    text: "🗑"
                                    color: "gray"
                                    font.pixelSize: 14
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        todoManager.removeTodo(index);
                                    }
                                }
                            }
                        }
                    }

                    // 为弹出列表添加滚动条
                    ScrollBar.vertical: ScrollBar {}
                }
            }
        }
    }

    // 登录状态相关对话框组件
    LoginStatusDialogs {
        id: loginStatusDialogs
        isDarkMode: widgetMode.isDarkMode
    }

    // 待办详情/编辑对话框组件
    TodoDetailsDialog {
        id: todoDetailsDialog
        isDarkMode: widgetMode.isDarkMode

        onTodoUpdated: function (index, todoData) {
            todoManager.updateTodo(index, todoData);
        }
    }

    // 待办事项下拉窗口
    Popup {
        id: todoItemDropdown

        property int calculatedY: {
            return mainContentPopup.y + mainContentPopup.height + 6;
        }

        // x: mainContentPopup.x
        y: calculatedY
        width: mainContentPopup.width
        height: 180
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        visible: isDesktopWidget && isShowDropdown

        property int currentTodoIndex: -1
        property var currentTodoData: null

        contentItem: Rectangle {
            color: theme.secondaryBackgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 5

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                // 顶部标题栏和收回按钮
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        text: todoItemDropdown.currentTodoData ? todoItemDropdown.currentTodoData.title : ""
                        font.bold: true
                        font.pixelSize: 14
                        color: theme.textColor
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    // 收回按钮
                    Rectangle {
                        width: 24
                        height: 24
                        color: "transparent"
                        border.width: 0
                        radius: 12

                        Text {
                            anchors.centerIn: parent
                            text: "^"
                            color: theme.textColor
                            font.pixelSize: 16
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                globalState.toggleDropdownVisible();
                            }
                            onEntered: {
                                parent.color = theme.borderColor;
                            }
                            onExited: {
                                parent.color = "transparent";
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: theme.borderColor
                }

                CustomButton {
                    text: "查看详情"
                    Layout.fillWidth: true
                    textColor: theme.textColor
                    backgroundColor: theme.backgroundColor
                    isDarkMode: widgetMode.isDarkMode
                    onClicked: {
                        globalState.toggleDropdownVisible();
                        todoDetailsDialog.openTodoDetails(todoItemDropdown.currentTodoData, todoItemDropdown.currentTodoIndex);
                    }
                }

                CustomButton {
                    text: "标记完成"
                    Layout.fillWidth: true
                    textColor: "white"
                    backgroundColor: theme.primaryColor
                    isDarkMode: widgetMode.isDarkMode
                    onClicked: {
                        todoManager.markAsDone(todoItemDropdown.currentTodoIndex);
                        globalState.toggleDropdownVisible();
                    }
                }

                CustomButton {
                    text: "删除任务"
                    Layout.fillWidth: true
                    textColor: "white"
                    backgroundColor: "#e74c3c"
                    isDarkMode: widgetMode.isDarkMode
                    onClicked: {
                        todoManager.removeTodo(todoItemDropdown.currentTodoIndex);
                        globalState.toggleDropdownVisible();
                    }
                }
            }
        }
    }
}
