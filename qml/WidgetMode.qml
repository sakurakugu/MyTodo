import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Item {
    id: widgetMode
    visible: globalState.isDesktopWidget

    property bool preventDragging: setting.get("setting/preventDragging", false) // 是否允许拖动

    // 外部导入的组件
    property var loginStatusDialogs
    property var todoCategoryManager

    /// 标题栏
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        width: 400
        height: 35
        color: theme.primaryColor
        border.width: 0                               ///< 边框宽度
        radius: 5                                     ///< 圆角
        visible: globalState.isDesktopWidget

        // 窗口拖拽处理区域
        WindowDragHandler {
            anchors.fill: parent
            targetWindow: root
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 5   ///< 左边距
            anchors.rightMargin: 5  ///< 右边距

            IconButton {
                text: "\ue90f"              ///< 菜单图标
                onClicked: globalState.toggleSettingsVisible()
                textColor: theme.textColor
                fontSize: 16
            }

            /// 待办状态指示器
            Text {
                id: todoStatusIndicator
                Layout.alignment: Qt.AlignVCenter
                color: theme.textColor
                font.pixelSize: 14
                font.bold: true

                // TODO: 新增或删除待办后，这里的文字没有更改（显示的是当前分类下的待办数量）
                property int todoCount: todoManager.rowCount()
                property bool isHovered: false

                text: {
                    if (isHovered) {
                        return todoCount > 0 ? todoCount + "个待办" : "没有待办";
                    } else {
                        return todoCount > 0 ? todoCount + "个待办" : "我的待办";
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.isHovered = true
                    onExited: parent.isHovered = false
                }
            }

            Item {
                Layout.fillWidth: true
            }

            /// 任务列表展开/收起按钮
            IconButton {
                text: globalState.isShowTodos ? "\ue667" : "\ue669"     ///< 根据状态显示箭头
                onClicked: globalState.toggleTodosVisible()     ///< 切换任务列表显示
                textColor: theme.textColor
                fontSize: 16
            }

            /// 添加任务按钮
            IconButton {
                text: "\ue903"                                 ///< 加号图标
                onClicked: globalState.toggleAddTaskVisible()   ///< 切换添加任务界面显示
                textColor: theme.textColor
                fontSize: 16
            }

            /// 普通模式和小组件模式切换按钮
            IconButton {
                text: "\ue620"
                /// 鼠标按下事件处理
                onClicked: {
                    globalState.toggleWidgetMode();
                }
                textColor: theme.textColor
                fontSize: 18
            }
        }
    }

    // 设置窗口
    Popup {
        id: settingsPopup
        y: 50
        width: 400
        height: 250
        padding: 0 // 消除Popup和contentItem之间的间隙

        modal: false // 非模态，允许同时打开多个弹出窗口
        focus: true
        closePolicy: Popup.NoAutoClose
        visible: globalState.isDesktopWidget && globalState.isShowSetting

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
                CustomSwitch {
                    id: darkModeCheckBox
                    text: "深色模式"
                    checked: globalState.isDarkMode

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
                        globalState.isDarkMode = checked;
                    }
                }

                CustomSwitch {
                    id: preventDraggingCheckBox
                    text: "防止拖动窗口（小窗口模式）"
                    checked: widgetMode.preventDragging
                    enabled: globalState.isDesktopWidget
                    onCheckedChanged: {
                        // 保存设置到配置文件
                        setting.save("setting/preventDragging", checked);
                        // 发出信号通知父组件
                        widgetMode.preventDragging = checked;
                    }
                }

                CustomSwitch {
                    id: autoSyncSwitch
                    text: "自动同步"
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
            }
        }
    }

    // 添加任务窗口
    Popup {
        id: addTaskPopup
        y: settingsPopup.visible ? settingsPopup.y + 250 + 6 : 50
        width: 400
        height: 250
        modal: false // 非模态，允许同时打开多个弹出窗口
        focus: true
        closePolicy: Popup.NoAutoClose
        visible: globalState.isDesktopWidget && globalState.isShowAddTask

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
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight

                    Button {
                        text: "添加"
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

    // 主内容区窗口
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
        visible: globalState.isDesktopWidget && globalState.isShowTodos // 在小组件模式下且需要显示所有任务时显示

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
                                color: model.isCompleted ? theme.completedColor : theme.lowImportantColor

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

    // 待办事项详情窗口
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
        visible: globalState.isDesktopWidget && globalState.isShowDropdown

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

                Button {
                    text: "查看详情"
                    Layout.fillWidth: true
                    onClicked: {
                        globalState.toggleDropdownVisible();
                        todoDetailsDialog.openTodoDetails(todoItemDropdown.currentTodoData, todoItemDropdown.currentTodoIndex);
                    }
                }

                Button {
                    text: "标记完成"
                    Layout.fillWidth: true
                    onClicked: {
                        todoManager.markAsDone(todoItemDropdown.currentTodoIndex);
                        globalState.toggleDropdownVisible();
                    }
                }

                Button {
                    text: "删除任务"
                    Layout.fillWidth: true
                    onClicked: {
                        todoManager.removeTodo(todoItemDropdown.currentTodoIndex);
                        globalState.toggleDropdownVisible();
                    }
                }
            }
        }
    }
}
