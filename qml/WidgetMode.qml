import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Item {
    id: toolMode
    visible: globalState.isDesktopWidget
    // 外部导入的组件
    property var loginStatusDialogs
    property var todoCategoryManager
    property var root

    
    property int spacing: 10 // 弹窗之间的间距
    
    // 动态计算总宽度和高度
    property int totalWidth: 400  // 固定宽度
    property int totalHeight: {
        var height = titleBar.height;  // 标题栏高度
        
        if (settingsPopup.visible) {
            height += spacing + settingsPopup.height;
        }
        if (addTaskPopup.visible) {
            height += spacing + addTaskPopup.height;
        }
        if (mainContentPopup.visible) {
            height += spacing + mainContentPopup.height;
        }
        if (todoItemDropdown.visible) {
            height += spacing + todoItemDropdown.height;
        }
        
        return height;
    }
    
    // 当尺寸变化时通知父窗口
    onTotalWidthChanged: {
        if (root && globalState.isDesktopWidget) {
            root.width = totalWidth;
        }
    }
    
    onTotalHeightChanged: {
        if (root && globalState.isDesktopWidget) {
            root.height = totalHeight;
        }
    }
    
    // 监听小组件模式状态变化
    Connections {
        target: globalState
        function onIsDesktopWidgetChanged() {
            if (globalState.isDesktopWidget && root) {
                // 切换到小组件模式时立即更新尺寸
                root.width = totalWidth;
                root.height = totalHeight;
            }
        }
    }
    
    // 组件完成时初始化尺寸
    Component.onCompleted: {
        if (globalState.isDesktopWidget && root) {
            root.width = totalWidth;
            root.height = totalHeight;
        }
    }

    // 标题栏
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        width: 400
        height: 35
        color: ThemeManager.primaryColor
        border.width: 0                               ///< 边框宽度
        radius: 5                                     ///< 圆角
        visible: globalState.isDesktopWidget

        // 窗口拖拽处理区域
        WindowDragHandler {
            anchors.fill: parent
            targetWindow: toolMode.root
            enabled: !globalState.preventDragging
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 5   ///< 左边距
            anchors.rightMargin: 5  ///< 右边距

            IconButton {
                text: "\ue90f"              ///< 菜单图标
                onClicked: globalState.toggleSettingsVisible()
            }

            /// 待办状态指示器
            Text {
                id: todoStatusIndicator
                Layout.alignment: Qt.AlignVCenter
                color: ThemeManager.textColor
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
            }

            /// 添加任务按钮
            IconButton {
                text: "\ue903"                                 ///< 加号图标
                onClicked: globalState.toggleAddTaskVisible()   ///< 切换添加任务界面显示
            }

            /// 普通模式和小组件模式切换按钮
            IconButton {
                text: "\ue620"
                /// 鼠标按下事件处理
                onClicked: {
                    globalState.toggleWidgetMode();
                }
                fontSize: 18
            }
        }
    }

    // 设置窗口
    Popup {
        id: settingsPopup
        y: titleBar.height + toolMode.spacing
        width: 400
        height: 250
        padding: 0 // 消除Popup和contentItem之间的间隙

        modal: false // 非模态，允许同时打开多个弹出窗口
        focus: true
        closePolicy: Popup.NoAutoClose
        visible: globalState.isDesktopWidget && globalState.isShowSetting

        contentItem: Rectangle {
            color: ThemeManager.secondaryBackgroundColor
            border.color: ThemeManager.borderColor
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
                    color: ThemeManager.textColor
                }

                // 设置内容
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
                    text: todoManager.isLoggedIn ? qsTr("自动同步") : qsTr("自动同步（未登录）")
                    checked: todoSyncServer.isAutoSyncEnabled
                    leftMargin: 10
                    controlType: ControlRow.ControlType.Switch

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
            }
        }
    }

    // 添加任务窗口
    Popup {
        id: addTaskPopup

        property int calculatedY: {
            var baseY = titleBar.height + toolMode.spacing;
            if (settingsPopup.visible) {
                baseY += settingsPopup.height + toolMode.spacing;
            }
            return baseY;
        }

        y: calculatedY
        width: 400
        height: 250
        modal: false // 非模态，允许同时打开多个弹出窗口
        focus: true
        closePolicy: Popup.NoAutoClose
        visible: globalState.isDesktopWidget && globalState.isShowAddTask

        // TODO: 这里可以天空标题输入框，还有一些详细设置
        contentItem: Rectangle {
            color: ThemeManager.secondaryBackgroundColor
            border.color: ThemeManager.borderColor
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
                    color: ThemeManager.textColor
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight

                    CustomButton {
                        text: "添加"
                        onClicked: todoManager.addTodo(qsTr("新的待办事项"))
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
            var baseY = titleBar.height + toolMode.spacing;
            if (settingsPopup.visible) {
                baseY += settingsPopup.height + toolMode.spacing;
            }
            if (addTaskPopup.visible) {
                baseY += addTaskPopup.height + toolMode.spacing;
            }
            return baseY;
        }
        y: calculatedY

        width: 400
        height: baseHeight
        modal: false // 非模态，允许同时打开多个弹出窗口
        focus: true
        closePolicy: Popup.NoAutoClose
        visible: globalState.isDesktopWidget && globalState.isShowTodos // 在小组件模式下且需要显示所有任务时显示

        contentItem: Rectangle {
            color: ThemeManager.secondaryBackgroundColor
            border.color: ThemeManager.borderColor
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
                    color: ThemeManager.textColor
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
                                color: ThemeManager.textColor
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
                        color: index % 2 === 0 ? ThemeManager.secondaryBackgroundColor : ThemeManager.backgroundColor

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
                                color: model.isCompleted ? ThemeManager.completedColor : ThemeManager.lowImportantColor

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
                                color: ThemeManager.textColor
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
            var baseY = titleBar.height + toolMode.spacing;
            if (settingsPopup.visible) {
                baseY += settingsPopup.height + toolMode.spacing;
            }
            if (addTaskPopup.visible) {
                baseY += addTaskPopup.height + toolMode.spacing;
            }
            if (mainContentPopup.visible) {
                baseY += mainContentPopup.height + toolMode.spacing;
            }
            return baseY;
        }
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
            color: ThemeManager.secondaryBackgroundColor
            border.color: ThemeManager.borderColor
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
                        color: ThemeManager.textColor
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
                            color: ThemeManager.textColor
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
                                parent.color = ThemeManager.borderColor;
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
                    color: ThemeManager.borderColor
                }

                CustomButton {
                    text: "查看详情"
                    Layout.fillWidth: true
                    onClicked: {
                        globalState.toggleDropdownVisible();
                        todoDetailsDialog.openTodoDetails(todoItemDropdown.currentTodoData, todoItemDropdown.currentTodoIndex);
                    }
                }

                CustomButton {
                    text: "标记完成"
                    Layout.fillWidth: true
                    onClicked: {
                        todoManager.markAsDone(todoItemDropdown.currentTodoIndex);
                        globalState.toggleDropdownVisible();
                    }
                }

                CustomButton {
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
