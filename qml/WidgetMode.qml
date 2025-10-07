/**
 * @file WidgetMode.qml
 * @brief 小组件模式组件
 *
 * 该组件用于创建应用的小组件模式，包含应用的设置页面、主内容页面和添加任务页面等。
 *
 * @author Sakurakugu
 * @date 2025-08-17 03:57:06(UTC+8) 周日
 * @change 2025-10-05 23:50:47(UTC+8) 周日
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Item {
    id: toolMode
    visible: globalData.isDesktopWidget
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
        if (root && globalData.isDesktopWidget) {
            root.width = totalWidth;
        }
    }

    onTotalHeightChanged: {
        if (root && globalData.isDesktopWidget) {
            root.height = totalHeight;
        }
    }

    // 监听小组件模式状态变化
    Connections {
        target: globalData
        function onIsDesktopWidgetChanged() {
            if (globalData.isDesktopWidget && root) {
                // 切换到小组件模式时立即更新尺寸
                root.width = totalWidth;
                root.height = totalHeight;
            }
        }
    }

    // 组件完成时初始化尺寸
    Component.onCompleted: {
        if (globalData.isDesktopWidget && root) {
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
        visible: globalData.isDesktopWidget

        // 窗口拖拽处理区域
        WindowDragHandler {
            anchors.fill: parent
            targetWindow: toolMode.root
            enabled: !globalData.preventDragging
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 5   ///< 左边距
            anchors.rightMargin: 5  ///< 右边距

            IconButton {
                text: "\ue90f"              ///< 菜单图标
                onClicked: globalData.toggleSettingsVisible()  ///< 切换设置页面显示
            }

            /// 待办状态指示器
            Text {
                id: todoStatusIndicator
                Layout.alignment: Qt.AlignVCenter
                color: ThemeManager.textColor
                font.pixelSize: 14
                font.bold: true

                property int todoCount: todoManager.todoModel.rowCount()
                property bool isHovered: false

                text: {
                    if (isHovered) {
                        return todoCount > 0 ? todoCount + "个待办" : "没有待办";
                    } else {
                        return todoCount > 0 ? todoCount + "个待办" : "我的待办";
                    }
                }

                // 监听模型变化，确保待办数量及时更新
                Connections {
                    target: todoManager.todoModel
                    function onRowsInserted() {
                        todoStatusIndicator.todoCount = todoManager.todoModel.rowCount();
                    }
                    function onRowsRemoved() {
                        todoStatusIndicator.todoCount = todoManager.todoModel.rowCount();
                    }
                    function onModelReset() {
                        todoStatusIndicator.todoCount = todoManager.todoModel.rowCount();
                    }
                    function onDataChanged() {
                        todoStatusIndicator.todoCount = todoManager.todoModel.rowCount();
                    }
                }

                // 监听查询条件变化，当分类筛选改变时更新数量
                Connections {
                    target: todoManager.queryer
                    function onQueryConditionsChanged() {
                        todoStatusIndicator.todoCount = todoManager.todoModel.rowCount();
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
                text: globalData.isShowTodos ? "\ue667" : "\ue669"     ///< 根据状态显示箭头
                onClicked: globalData.toggleTodosVisible()     ///< 切换任务列表显示
            }

            /// 添加任务按钮
            IconButton {
                text: "\ue903"                                 ///< 加号图标
                onClicked: {
                    if (globalData.isShowAddTask) {
                        globalData.isNew = false;
                    } else {
                        globalData.isNew = true;
                    }
                    globalData.selectedTodo = null;
                    globalData.toggleAddTaskVisible();   ///< 切换添加任务界面显示
                }
            }

            /// 普通模式和小组件模式切换按钮
            IconButton {
                text: "\ue620"
                /// 鼠标按下事件处理
                onClicked: {
                    globalData.toggleWidgetMode();
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
        visible: globalData.isDesktopWidget && globalData.isShowSetting

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
                checked: globalData.isDarkMode
                enabled: !globalData.isFollowSystemDarkMode
                leftMargin: 10
                controlType: ControlRow.ControlType.Switch

                onCheckedChanged: {
                    globalData.isDarkMode = checked;
                    // 保存设置到配置文件
                    setting.save("setting/isDarkMode", checked);
                }
            }

            ControlRow {
                id: preventDraggingCheckBox
                text: qsTr("防止拖动窗口（小窗口模式）")
                checked: globalData.preventDragging
                enabled: globalData.isDesktopWidget
                leftMargin: 10
                controlType: ControlRow.ControlType.Switch
                onCheckedChanged: {
                    globalData.preventDragging = checked;
                    setting.save("setting/preventDragging", globalData.preventDragging);
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
        visible: globalData.isDesktopWidget && globalData.isShowTodos // 在小组件模式下且需要显示所有任务时显示
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
        visible: globalData.isDesktopWidget && globalData.isShowAddTask

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

            RowLayout {
                CustomTextInput {
                    id: titleInput
                    text: globalData.selectedTodo ? (globalData.selectedTodo.title || qsTr("无标题")) : qsTr("新建待办")
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
                        globalData.isShowAddTask = !globalData.isShowAddTask;
                        globalData.isNew = true;
                    }
                }
            }

            CustomTextEdit {
                id: contentInput
                Layout.fillWidth: true
                Layout.fillHeight: true
                placeholderText: qsTr("输入详情")
                text: globalData.selectedTodo ? (globalData.selectedTodo.description || "") : ""
                wrapMode: TextEdit.WrapAnywhere
                clip: true

                onEditingFinished: {
                    saveDescriptionIfChanged();
                }

                function saveDescriptionIfChanged() {
                    if (globalData.isNew) {
                        return;
                    }
                    if (globalData.selectedTodo && text !== globalData.selectedTodo.description) {
                        todoManager.updateTodo(globalData.selectedTodo.index, "description", text);
                        globalData.selectedTodo.description = text;
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
                            deadlineDatePicker.selectedDate = globalData.selectedTodo && globalData.selectedTodo.deadline ? globalData.selectedTodo.deadline : new Date();
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
                            if (globalData.isNew) {
                                return;
                            } else {
                                startDatePicker.selectedDate = globalData.selectedTodo && globalData.selectedTodo.recurrenceStartDate ? globalData.selectedTodo.recurrenceStartDate : new Date();
                                startDatePicker.open();
                            }
                        }
                    }

                    // 完成状态
                    IconButton {
                        text: "\ue8eb"
                        visible: !globalData.isNew
                        property bool checked: globalData.selectedTodo ? (globalData.selectedTodo.isCompleted || false) : false
                        onClicked: {
                            checked = !checked;
                            if (globalData.selectedTodo && checked !== globalData.selectedTodo.isCompleted) {
                                todoManager.markAsDone(globalData.selectedTodo.index.checked);
                                globalData.selectedTodo.isCompleted = checked;
                            }
                        }
                    }

                    // 重要程度
                    IconButton {
                        text: "\ue8de"
                        property bool checked: globalData.selectedTodo ? (globalData.selectedTodo.important || false) : false
                        onClicked: {
                            checked = !checked;
                            if (globalData.isNew) {
                                globalData.selectedTodo.important = checked;
                            } else if (globalData.selectedTodo && checked !== globalData.selectedTodo.important) {
                                todoManager.updateTodo(globalData.selectedTodo.index, "important", checked);
                                globalData.selectedTodo.important = checked;
                            }
                        }
                    }

                    // 删除按钮
                    IconButton {
                        text: "\ue8f5"
                        visible: !globalData.isNew
                        onClicked: {
                            if (globalData.selectedTodo) {
                                todoManager.markAsRemove(globalData.selectedTodo.index);
                                globalData.selectedTodo = null;
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    CustomButton {
                        text: globalData.isNew ? qsTr("添加") : qsTr("保存")
                        onClicked: {
                            if (globalData.isNew) {
                                todoManager.addTodo(titleInput.text, contentInput.text);
                            } else {
                                todoManager.updateTodo(globalData.selectedTodo.index, "title", titleInput.text);
                                todoManager.updateTodo(globalData.selectedTodo.index, "description", contentInput.text);
                                todoManager.updateTodo(globalData.selectedTodo.index, "deadline", globalData.selectedTodo.deadline);
                                todoManager.updateTodo(globalData.selectedTodo.index, "recurrenceCount", globalData.selectedTodo.recurrenceCount);
                                todoManager.updateTodo(globalData.selectedTodo.index, "recurrenceStartDate", globalData.selectedTodo.recurrenceStartDate);
                                todoManager.updateTodo(globalData.selectedTodo.index, "recurrenceInterval", globalData.selectedTodo.recurrenceInterval);
                                todoManager.updateTodo(globalData.selectedTodo.index, "important", globalData.selectedTodo.important);
                                todoManager.updateTodo(globalData.selectedTodo.index, "category", globalData.selectedTodo.category);
                                todoManager.markAsDone(globalData.selectedTodo.index, globalData.selectedTodo.isCompleted);
                            }
                            globalData.selectedTodo = null;
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
                value: globalData.selectedTodo ? globalData.selectedTodo.recurrenceCount : 0
                enabled: globalData.selectedTodo !== null && todoManager.queryer.currentFilter !== "recycle" && todoManager.queryer.currentFilter !== "done"
                implicitWidth: 100
                implicitHeight: 25

                onValueChanged: {
                    if (globalData.selectedTodo && value !== globalData.selectedTodo.recurrenceCount) {
                        todoManager.updateTodo(globalData.selectedTodo.index, "recurrenceCount", value);
                        globalData.selectedTodo.recurrenceCount = value;
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
                value: globalData.selectedTodo ? globalData.selectedTodo.recurrenceInterval : 0
                enabled: globalData.selectedTodo !== null && todoManager.queryer.currentFilter !== "recycle" && todoManager.queryer.currentFilter !== "done"

                onIntervalChanged: function (newValue) {
                    if (globalData.isNew) {
                        globalData.selectedTodo.recurrenceInterval = newValue;
                    } else if (globalData.selectedTodo && newValue !== globalData.selectedTodo.recurrenceInterval) {
                        todoManager.updateTodo(globalData.selectedTodo.index, "recurrenceInterval", newValue);
                        globalData.selectedTodo.recurrenceInterval = newValue;
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
            if (globalData.isNew) {
                globalData.selectedTodo.recurrenceStartDate = selectedDate;
            } else if (globalData.selectedTodo) {
                todoManager.updateTodo(globalData.selectedTodo.index, "recurrenceStartDate", selectedDate);
                globalData.selectedTodo.recurrenceStartDate = selectedDate;
            }
        }
    }

    // 截止日期选择器
    DateTimePicker {
        id: deadlineDatePicker

        onConfirmed: {
            if (globalData.isNew) {
                globalData.selectedTodo.deadline = selectedDate;
            } else if (globalData.selectedTodo) {
                todoManager.updateTodo(globalData.selectedTodo.index, "deadline", selectedDate);
                globalData.selectedTodo.deadline = selectedDate;
            }
        }
    }
}
