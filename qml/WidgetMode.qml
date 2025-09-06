/**
 * @brief 小组件模式组件
 *
 * 该组件用于创建应用的小组件模式，包含应用的设置页面、主内容页面和添加任务页面等。
 *
 * @author Sakurakugu
 * @date 2025-08-17 03:57:06(UTC+8) 周日
 * @change 2025-09-06 16:55:02(UTC+8) 周六
 * @version 0.4.0
 */
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
        if (mainContentPopup.visible) {
            height += spacing + mainContentPopup.height;
        }
        if (addTaskPopup.visible) {
            height += spacing + addTaskPopup.height;
        }

        return height;
    }

    function calculateStackedY(popup) {
        var y = titleBar.height + toolMode.spacing;

        // 所有 Popup 按显示顺序放在数组里
        var popupsInOrder = [ // 索引
            settingsPopup // 0
            , mainContentPopup // 1
            , addTaskPopup // 2
        ];

        for (var i = 0; i < popupsInOrder.length; i++) {
            var p = popupsInOrder[i];
            if (p === popup)
                break;  // 到当前 Popup 就停
            if (p.visible) {
                y += p.height + toolMode.spacing;
            }
        }

        return y;
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
                onClicked: {
                    if (globalState.isShowAddTask) {
                        globalState.isNew = false;
                    } else {
                        globalState.isNew = true;
                    }
                    globalState.selectedTodo = null;
                    globalState.toggleAddTaskVisible();   ///< 切换添加任务界面显示
                }
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
    CustomPopup {
        id: settingsPopup
        y: toolMode.calculateStackedY(settingsPopup)
        width: 400
        height: 200
        visible: globalState.isDesktopWidget && globalState.isShowSetting

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

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

    // 主内容区窗口
    CustomPopup {
        id: mainContentPopup
        y: toolMode.calculateStackedY(mainContentPopup)
        width: 400
        height: baseHeight
        visible: globalState.isDesktopWidget && globalState.isShowTodos // 在小组件模式下且需要显示所有任务时显示
        property int baseHeight: 275 // 一个待办事项的高度为50，其他的占了75px的高度

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
            TodoListContainer {}
        }
    }

    // 添加任务与显示详情窗口
    CustomPopup {
        id: addTaskPopup
        y: toolMode.calculateStackedY(addTaskPopup)
        width: 400
        height: 250
        visible: globalState.isDesktopWidget && globalState.isShowAddTask

        // TODO: 这里可以天空标题输入框，还有一些详细设置
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

            RowLayout {
                CustomTextInput {
                    id: titleInput
                    text: globalState.selectedTodo ? (globalState.selectedTodo.title || qsTr("无标题")) : qsTr("新建待办")
                    font.bold: true
                    font.pixelSize: 16
                    borderWidth: 0
                    Layout.fillWidth: true
                    backgroundColor: "transparent"
                }

                // 关闭按钮
                IconButton {
                    text: "\ue8d1"
                    onClicked: {
                        globalState.isShowAddTask = !globalState.isShowAddTask;
                        globalState.isNew = true;
                    }
                }
            }

            CustomTextEdit {
                id: contentInput
                Layout.fillWidth: true
                Layout.fillHeight: true
                placeholderText: qsTr("输入详情")
                text: globalState.selectedTodo ? (globalState.selectedTodo.description || "") : ""
                wrapMode: TextEdit.WrapAnywhere
                clip: true

                onEditingFinished: {
                    saveDescriptionIfChanged();
                }

                function saveDescriptionIfChanged() {
                    if (globalState.isNew) {
                        return;
                    }
                    if (globalState.selectedTodo && text !== globalState.selectedTodo.description) {
                        todoManager.updateTodo(globalState.selectedTodo.index, "description", text);
                        globalState.selectedTodo.description = text;
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                RowLayout {
                    Layout.alignment: Qt.AlignLeft

                    // 选择分类
                    IconButton {
                        text: "\ue605"
                        onClicked: {
                            var pos = mapToItem(null, 0, height);
                            todoCategoryManager.popup(pos.x, pos.y, false);
                        }
                    }

                    // 截止日期
                    IconButton {
                        text: "\ue6e5"
                        onClicked: {
                            deadlineDatePicker.selectedDate = globalState.selectedTodo && globalState.selectedTodo.deadline ? globalState.selectedTodo.deadline : new Date();
                            deadlineDatePicker.open();
                        }
                    }

                    // 重复天数
                    IconButton {
                        text: "\ue8ef"
                        onClicked: recurrenceIntervalRowDialog.open()
                    }

                    // 重复次数
                    IconButton {
                        text: "\ue601"
                        onClicked: recurrenceCountRowDialog.open()
                    }

                    // 重复开始日期
                    IconButton {
                        text: "\ue74b"
                        onClicked: {
                            if (globalState.isNew) {
                                return;
                            } else {
                                startDatePicker.selectedDate = globalState.selectedTodo && globalState.selectedTodo.recurrenceStartDate ? globalState.selectedTodo.recurrenceStartDate : new Date();
                                startDatePicker.open();
                            }
                        }
                    }

                    // 完成状态
                    IconButton {
                        text: "\ue8eb"
                        visible: !globalState.isNew
                        property bool checked: globalState.selectedTodo ? (globalState.selectedTodo.completed || false) : false
                        onClicked: {
                            checked = !checked;
                            if (globalState.selectedTodo && checked !== globalState.selectedTodo.completed) {
                                todoManager.updateTodo(globalState.selectedTodo.index, "isCompleted", checked);
                                globalState.selectedTodo.completed = checked;
                            }
                        }
                    }

                    // 重要程度
                    IconButton {
                        text: "\ue8de"
                        property bool checked: globalState.selectedTodo ? (globalState.selectedTodo.important || false) : false
                        onClicked: {
                            checked = !checked;
                            if (globalState.isNew) {
                                globalState.selectedTodo.important = checked;
                            } else if (globalState.selectedTodo && checked !== globalState.selectedTodo.important) {
                                todoManager.updateTodo(globalState.selectedTodo.index, "important", checked);
                                globalState.selectedTodo.important = checked;
                            }
                        }
                    }

                    // 删除按钮
                    IconButton {
                        text: "\ue8f5"
                        visible: !globalState.isNew
                        onClicked: {
                            if (globalState.selectedTodo) {
                                todoManager.removeTodo(globalState.selectedTodo.index);
                                globalState.selectedTodo = null;
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    CustomButton {
                        text: globalState.isNew ? qsTr("添加") : qsTr("保存")
                        onClicked: {
                            if (globalState.isNew) {
                                // TODO：c++中有些没实现
                                todoManager.addTodo(titleInput.text, contentInput.text);
                            } else {
                                todoManager.updateTodo(globalState.selectedTodo.index, "title", titleInput.text);
                                todoManager.updateTodo(globalState.selectedTodo.index, "description", contentInput.text);
                                todoManager.updateTodo(globalState.selectedTodo.index, "deadline", globalState.selectedTodo.deadline);
                                todoManager.updateTodo(globalState.selectedTodo.index, "recurrenceCount", globalState.selectedTodo.recurrenceCount);
                                todoManager.updateTodo(globalState.selectedTodo.index, "recurrenceStartDate", globalState.selectedTodo.recurrenceStartDate);
                                todoManager.updateTodo(globalState.selectedTodo.index, "recurrenceInterval", globalState.selectedTodo.recurrenceInterval);
                                todoManager.updateTodo(globalState.selectedTodo.index, "important", globalState.selectedTodo.important);
                                todoManager.updateTodo(globalState.selectedTodo.index, "category", globalState.selectedTodo.category);
                                todoManager.updateTodo(globalState.selectedTodo.index, "completed", globalState.selectedTodo.completed);
                            }
                            globalState.selectedTodo = null;
                        }
                    }
                }
            }
        }
    }
    BaseDialog {
        id: recurrenceCountRowDialog
        dialogTitle: "重复次数"
        RowLayout {
            Layout.alignment: Qt.AlignCenter
            IconButton {
                text: "\ue601"
            }

            Text {
                text: qsTr("共")
                font.pixelSize: 16
                color: ThemeManager.textColor
                verticalAlignment: Text.AlignVCenter
            }

            CustomSpinBox {
                id: drawerCountSpinBox
                from: 0
                to: 999
                value: globalState.selectedTodo ? globalState.selectedTodo.recurrenceCount : 0
                enabled: globalState.selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"
                implicitWidth: 100
                implicitHeight: 25

                onValueChanged: {
                    if (globalState.selectedTodo && value !== globalState.selectedTodo.recurrenceCount) {
                        todoManager.updateTodo(globalState.selectedTodo.index, "recurrenceCount", value);
                        globalState.selectedTodo.recurrenceCount = value;
                    }
                }
            }

            Text {
                text: qsTr("次")
                font.pixelSize: 16
                color: ThemeManager.textColor
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    BaseDialog {
        id: recurrenceIntervalRowDialog
        dialogTitle: "重复天数"

        RowLayout {
            Layout.alignment: Qt.AlignCenter
            IconButton {
                text: "\ue8ef"
            }
            Text {
                text: qsTr("重复:")
                font.pixelSize: 16
                color: ThemeManager.textColor
                verticalAlignment: Text.AlignVCenter
            }

            RecurrenceSelector {
                id: drawerIntervalSelector
                Layout.fillWidth: true
                value: globalState.selectedTodo ? globalState.selectedTodo.recurrenceInterval : 0
                enabled: globalState.selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"

                onIntervalChanged: function (newValue) {
                    if (globalState.isNew) {
                        globalState.selectedTodo.recurrenceInterval = newValue;
                    } else if (globalState.selectedTodo && newValue !== globalState.selectedTodo.recurrenceInterval) {
                        todoManager.updateTodo(globalState.selectedTodo.index, "recurrenceInterval", newValue);
                        globalState.selectedTodo.recurrenceInterval = newValue;
                    }
                }
            }
        }
    }

    // 开始日期选择器
    DateTimePicker {
        id: startDatePicker
        enableTimeMode: false

        onConfirmed: {
            if (globalState.isNew) {
                globalState.selectedTodo.recurrenceStartDate = selectedDate;
            } else if (globalState.selectedTodo) {
                todoManager.updateTodo(globalState.selectedTodo.index, "recurrenceStartDate", selectedDate);
                globalState.selectedTodo.recurrenceStartDate = selectedDate;
            }
        }
    }

    // 截止日期选择器
    DateTimePicker {
        id: deadlineDatePicker

        onConfirmed: {
            if (globalState.isNew) {
                globalState.selectedTodo.deadline = selectedDate;
            } else if (globalState.selectedTodo) {
                todoManager.updateTodo(globalState.selectedTodo.index, "deadline", selectedDate);
                globalState.selectedTodo.deadline = selectedDate;
            }
        }
    }
}
