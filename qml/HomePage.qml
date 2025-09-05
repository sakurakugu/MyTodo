import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Page {
    id: homePage

    property var root
    property var stackView
    property var selectedTodo: null       // 当前选中的待办事项
    property bool multiSelectMode: false  // 多选模式
    property var selectedItems: []        // 选中的项目索引列表

    // 外部导入的组件
    property var settingPage
    property var loginStatusDialogs
    property var todoCategoryManager

    // 组件完成时设置默认过滤器为"未完成"（待办）
    Component.onCompleted: {
        todoFilter.currentFilter = "todo";
    }

    background: MainBackground {}

    // 主布局
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧功能区
        Rectangle {
            Layout.preferredWidth: 60
            Layout.fillHeight: true
            color: ThemeManager.backgroundColor
            border.width: 1
            border.color: ThemeManager.borderColor
            topLeftRadius: 10
            bottomLeftRadius: 10

            // 窗口拖拽处理区域
            WindowDragHandler {
                anchors.fill: parent
                targetWindow: root
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                // 用户头像
                MouseArea {
                    id: userProfileMouseArea
                    Layout.preferredWidth: userProfileContent.width
                    Layout.preferredHeight: userProfileContent.height
                    Layout.alignment: Qt.AlignVCenter
                    z: 2  // 确保在拖拽MouseArea之上

                    /// 点击弹出用户菜单
                    onClicked: {
                        // 计算菜单位置，固定在用户头像右侧
                        var pos = mapToItem(null, width, height);
                        userMenu.popup(pos.x, pos.y);
                    }

                    RowLayout {
                        id: userProfileContent
                        spacing: 10

                        // 用户头像框
                        Rectangle {
                            width: 30
                            height: 30
                            radius: 15                               ///< 圆形头像
                            color: "white"                           ///< 不改变颜色
                            Layout.alignment: Qt.AlignVCenter        ///< 垂直居中对齐
                            border.width: 1
                            border.color: ThemeManager.borderColor

                            // 头像图标
                            // TODO：从用户信息中获取头像图标
                            Text {
                                anchors.centerIn: parent
                                text: "👤"                      ///< 默认用户图标
                                font.pixelSize: 18
                            }
                        }
                    }
                }

                // 全部、未完成、已完成、回收站
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: implicitHeight
                    implicitHeight: column.implicitHeight + 20  // Column高度 + 上下边距
                    color: ThemeManager.borderColor
                    radius: 50

                    Column {
                        id: column
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        IconButton {
                            text: "\ue8df"              ///< 全部图标
                            onClicked: todoFilter.currentFilter = "all"
                            width: parent.width
                        }

                        IconButton {
                            text: "\ue7fc"              ///< 未完成图标
                            onClicked: todoFilter.currentFilter = "todo"
                            width: parent.width
                        }

                        IconButton {
                            text: "\ue7fa"              ///< 已完成图标
                            onClicked: todoFilter.currentFilter = "done"
                            width: parent.width
                        }

                        IconButton {
                            text: "\ue922"              ///< 回收站图标
                            onClicked: todoFilter.currentFilter = "recycle"
                            width: parent.width
                        }
                    }
                }

                // 排序方向
                IconButton {
                    property bool isDescending: false            // 排序方向
                    text: isHovered ? (isDescending ? "\ue8c3" : "\ue8c4") : ("\ue8c5")
                    onClicked: {
                        isDescending = !isDescending;
                        todoSorter.setDescending(isDescending);
                    }
                    width: parent.width
                }

                // 筛选菜单
                IconButton {
                    id: filterButton
                    Layout.alignment: Qt.AlignHCenter
                    text: "\ue8db"              ///< 筛选图标
                    onClicked: {
                        // 计算菜单位置，固定在筛选按钮右侧
                        var pos = mapToItem(null, width, height);
                        filterMenu.popup(pos.x, pos.y);
                    }
                }

                //  种类筛选
                IconButton {
                    id: categoryFilterButton
                    Layout.alignment: Qt.AlignHCenter
                    text: "\ue90f"              ///< 筛选图标
                    onClicked: {
                        // 计算菜单位置，固定在筛选按钮右侧
                        var pos = mapToItem(null, width, height);
                        todoCategoryManager.popup(pos.x, pos.y, true);
                    }
                }

                // 标题栏剩余空间填充
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                // 刷新
                IconButton {
                    id: refreshButton
                    text: "\ue8e2"              ///< 刷新图标

                    // 添加旋转动画
                    RotationAnimation {
                        id: refreshButtonAnimation
                        target: refreshButton
                        from: 0
                        to: 360
                        duration: 1000
                        running: false
                    }

                    onClicked: {
                        if (!globalState.refreshing) {
                            globalState.refreshing = true;
                            refreshButtonAnimation.start();  // 开始旋转动画
                            todoManager.syncWithServer();
                        }
                    }
                }

                // 深色模式
                IconButton {
                    text: globalState.isDarkMode ? "\ue668" : "\ue62e"
                    onClicked: {
                        globalState.isDarkMode = !globalState.isDarkMode;
                        setting.save("setting/isDarkMode", globalState.isDarkMode);
                    }
                }

                // 设置
                IconButton {
                    text: "\ue913"              ///< 设置图标
                    onClicked: homePage.stackView.push(homePage.settingPage)
                }
            }
        }

        // 中间待办事项列表区域
        Rectangle {
            Layout.preferredWidth: 210
            Layout.fillHeight: true
            color: ThemeManager.backgroundColor

            // 上边框
            Divider {
                isTopOrLeft: true
            }

            TodoListContainer {
                root: homePage.root
                anchors.topMargin: 1
                anchors.bottomMargin: 1
            }

            // 下边框
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                color: ThemeManager.borderColor
                height: 1
            }
        }

        // 右侧详情区域
        Rectangle {
            id: detailArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            border.width: 1
            border.color: ThemeManager.borderColor
            color: ThemeManager.backgroundColor
            topRightRadius: 10
            bottomRightRadius: 10

            // 抽屉显示状态
            property bool drawerVisible: false

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 窗口标题栏
                Rectangle {
                    id: titleBar
                    Layout.preferredHeight: 30
                    Layout.fillWidth: true
                    color: "transparent"

                    // 窗口拖拽处理区域
                    WindowDragHandler {
                        anchors.fill: parent
                        targetWindow: root
                    }

                    // 小工具切换按钮、最小化、最大化/恢复、关闭按钮
                    RowLayout {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: 30
                        spacing: 0

                        /// 普通模式和小组件模式切换按钮
                        IconButton {
                            text: "\ue61f"
                            /// 鼠标按下事件处理
                            onClicked: {
                                globalState.toggleWidgetMode();
                            }
                            fontSize: 18
                        }

                        // 最小化按钮
                        IconButton {
                            text: "\ue65a"
                            onClicked: root.showMinimized()
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
                        }

                        // 关闭按钮
                        IconButton {
                            text: "\ue8d1"
                            onClicked: root.close()
                        }
                    }
                }

                // 详情标题栏
                Rectangle {
                    id: detailTitleBar
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: ThemeManager.backgroundColor

                    // 左边框
                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: ThemeManager.borderColor
                    }

                    // 窗口拖拽处理区域
                    WindowDragHandler {
                        anchors.fill: parent
                        targetWindow: root
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8

                        // 标题栏输入框
                        CustomTextInput {
                            id: titleField
                            text: selectedTodo ? (selectedTodo.title || qsTr("无标题")) : qsTr("选择一个待办事项")
                            font.pixelSize: 18
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            font.bold: true
                            enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done" // 只有选中待办事项且不在回收站或已完成模式时才能编辑
                            borderWidth: 0
                            backgroundColor: "transparent"

                            // 保存标题的函数
                            function saveTitleIfChanged() {
                                if (selectedTodo && text !== selectedTodo.title) {
                                    // 通过TodoManager的updateTodo方法保存更改
                                    todoManager.updateTodo(selectedTodo.index, "title", text);
                                    // 更新本地selectedTodo对象以保持UI同步
                                    selectedTodo.title = text;
                                }
                            }

                            // 编辑完成后保存并移动焦点
                            onEditingFinished: {
                                saveTitleIfChanged();
                                // 将焦点移动到详情TextEdit
                                descriptionField.forceActiveFocus();
                            }
                        }

                        // 更多操作按钮
                        IconButton {
                            text: "\ue955"                      ///< 更多操作图标
                            onClicked: {
                                // 切换抽屉显示状态
                                detailArea.drawerVisible = !detailArea.drawerVisible;
                            }
                        }
                    }

                    // 右边框
                    Rectangle {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: ThemeManager.borderColor
                    }

                    // 底部边框
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: ThemeManager.borderColor
                    }
                }

                // 时间和分类信息栏
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: {
                        if (!selectedTodo)
                            return 0;
                        return 32;
                    }
                    visible: selectedTodo !== null
                    color: ThemeManager.backgroundColor

                    // 左边框
                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: ThemeManager.borderColor
                    }

                    // 时间和分类行
                    RowLayout {
                        anchors.fill: parent
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 16

                        // 时间显示
                        Text {
                            text: isCreatedText ? createdText : timeText
                            font.pixelSize: 12
                            color: ThemeManager.textColor
                            verticalAlignment: Text.AlignVCenter
                            Layout.alignment: Qt.AlignLeft
                            Layout.leftMargin: 8

                            property bool isCreatedText: false
                            property string timeText: {
                                if (!selectedTodo)
                                    return "";
                                if (todoFilter.currentFilter === "recycle") {
                                    return selectedTodo.deletedAt ? qsTr("删除时间: ") + Qt.formatDateTime(selectedTodo.deletedAt, "yyyy-MM-dd hh:mm") : "";
                                } else if (todoFilter.currentFilter === "done") {
                                    return selectedTodo.completedAt ? qsTr("完成时间: ") + Qt.formatDateTime(selectedTodo.completedAt, "yyyy-MM-dd hh:mm") : "";
                                } else {
                                    return selectedTodo.lastModifiedAt ? qsTr("修改时间: ") + Qt.formatDateTime(selectedTodo.lastModifiedAt, "yyyy-MM-dd hh:mm") : "";
                                }
                            }
                            property string createdText: {
                                if (!selectedTodo)
                                    return "";
                                return selectedTodo.createdAt ? qsTr("创建时间: ") + Qt.formatDateTime(selectedTodo.createdAt, "yyyy-MM-dd hh:mm") : "";
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                // 点击时切换时间显示
                                onClicked: {
                                    parent.isCreatedText = !parent.isCreatedText;
                                }
                                // 悬浮时切换时间显示
                                onEntered: {
                                    parent.isCreatedText = true;
                                }
                                // 离开时切换时间显示
                                onExited: {
                                    parent.isCreatedText = false;
                                }
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        // 分类显示和选择
                        RowLayout {
                            Layout.rightMargin: 8
                            spacing: 8

                            // TODO: 改成图标
                            Text {
                                text: qsTr("分类:")
                                font.pixelSize: 12
                                color: ThemeManager.textColor
                                verticalAlignment: Text.AlignVCenter
                            }

                            CustomButton {
                                text: {
                                    if (!selectedTodo)
                                        return qsTr("未分类");
                                    return selectedTodo.category || qsTr("未分类");
                                }
                                font.pixelSize: 12
                                implicitHeight: 30
                                implicitWidth: 70
                                enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"
                                onClicked: {
                                    var pos = mapToItem(null, 0, height);
                                    todoCategoryManager.popup(pos.x, pos.y, false);
                                }
                                is2ndColor: true
                            }
                        }
                    }

                    // 右边框
                    Rectangle {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: ThemeManager.borderColor
                    }

                    // 底部边框
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: ThemeManager.borderColor
                    }
                }

                // 详情内容
                CustomTextEdit {
                    id: descriptionField
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 8
                    placeholderText: qsTr("输入详情")
                    text: selectedTodo ? (selectedTodo.description || "") : ""
                    wrapMode: TextEdit.WrapAnywhere
                    enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"

                    onEditingFinished: {
                        saveDescriptionIfChanged();
                    }

                    function saveDescriptionIfChanged() {
                        if (selectedTodo && text !== selectedTodo.description) {
                            todoManager.updateTodo(selectedTodo.index, "description", text);
                            selectedTodo.description = text;
                        }
                    }
                }

                // 工具栏
                // TODO: 还有好多没实现
                Rectangle {
                    Layout.fillWidth: true
                    height: 20
                    color: "transparent"
                    visible: selectedTodo !== null

                    Row {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 8
                        spacing: 10

                        // 字数显示
                        Text {
                            text: descriptionField.text.length + " " + qsTr("字符")
                            font.pixelSize: 12
                            color: ThemeManager.textColor
                            opacity: 0.7
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            // 抽屉组件遮罩层
            Rectangle {
                id: overlay
                anchors.fill: parent
                anchors.topMargin: titleBar.height + detailTitleBar.height - 1
                color: "transparent"
                opacity: 0.3
                z: 99
                visible: detailArea.drawerVisible

                Behavior on opacity {
                    NumberAnimation {
                        duration: 200
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: detailArea.drawerVisible = false
                }
            }

            // 抽屉组件
            Rectangle {
                id: drawer
                anchors.top: parent.top
                anchors.topMargin: titleBar.height + detailTitleBar.height - 1
                anchors.bottom: parent.bottom
                width: 300
                color: ThemeManager.backgroundColor
                border.width: 1
                border.color: ThemeManager.borderColor
                z: 100  // 确保在最上层

                // 抽屉显示/隐藏动画
                x: detailArea.drawerVisible ? detailArea.width - width : detailArea.width

                Behavior on x {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutCubic
                    }
                }

                // 抽屉内容
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16

                    // 抽屉标题栏
                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: qsTr("详细设置")
                            font.pixelSize: 16
                            font.bold: true
                            color: ThemeManager.textColor
                            Layout.fillWidth: true
                        }

                        // 关闭按钮
                        IconButton {
                            text: "\ue8d1"
                            onClicked: {
                                detailArea.drawerVisible = false;
                            }
                        }
                    }

                    // 分隔线
                    Divider {}

                    // 待办事项属性编辑区域
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ColumnLayout {
                            width: parent.width - 20
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            spacing: 12

                            // 分类显示和选择
                            RowLayout {
                                spacing: 8
                                Layout.alignment: Qt.AlignVCenter

                                IconButton {
                                    text: "\ue605"
                                    fontSize: 18
                                }
                                Text {
                                    text: qsTr("分类:")
                                    font.pixelSize: 16
                                    color: ThemeManager.textColor
                                    verticalAlignment: Text.AlignVCenter
                                }

                                CustomButton {
                                    text: {
                                        if (!selectedTodo)
                                            return qsTr("未分类");
                                        return selectedTodo.category || qsTr("未分类");
                                    }
                                    font.pixelSize: 12
                                    implicitHeight: 40
                                    enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"
                                    onClicked: {
                                        var pos = mapToItem(null, 0, height);
                                        todoCategoryManager.popup(pos.x, pos.y, false);
                                    }

                                    is2ndColor: true
                                }
                            }

                            // 截止日期
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                IconButton {
                                    text: "\ue6e5"
                                }

                                Text {
                                    text: qsTr("截止日期:")
                                    font.pixelSize: 16
                                    color: ThemeManager.textColor
                                    verticalAlignment: Text.AlignVCenter
                                }

                                CustomButton {
                                    id: drawerDeadlineField
                                    text: {
                                        if (selectedTodo && selectedTodo.deadline && !isNaN(selectedTodo.deadline.getTime()))
                                            return Qt.formatDateTime(selectedTodo.deadline, "yyyy-MM-dd hh:mm");
                                        return qsTr("点击选择日期");
                                    }
                                    enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"
                                    font.pixelSize: 12
                                    Layout.fillWidth: true
                                    implicitHeight: 40
                                    is2ndColor: true

                                    onClicked: {
                                        deadlineDatePicker.selectedDate = selectedTodo && selectedTodo.deadline ? selectedTodo.deadline : new Date();
                                        deadlineDatePicker.open();
                                        console.log(selectedTodo.deadline);
                                    }
                                }
                            }

                            // 重复天数
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

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
                                    value: selectedTodo ? selectedTodo.recurrenceInterval : 0
                                    enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"

                                    onIntervalChanged: function(newValue) {
                                        if (selectedTodo && newValue !== selectedTodo.recurrenceInterval) {
                                            todoManager.updateTodo(selectedTodo.index, "recurrenceInterval", newValue);
                                            selectedTodo.recurrenceInterval = newValue;
                                        }
                                    }
                                }
                            }

                            // 重复次数
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

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
                                    value: selectedTodo ? selectedTodo.recurrenceCount : 0
                                    enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"
                                    implicitWidth: 100
                                    implicitHeight: 25

                                    onValueChanged: {
                                        if (selectedTodo && value !== selectedTodo.recurrenceCount) {
                                            todoManager.updateTodo(selectedTodo.index, "recurrenceCount", value);
                                            selectedTodo.recurrenceCount = value;
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

                            // 重复开始日期
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                IconButton {
                                    text: "\ue74b"
                                }

                                Text {
                                    text: qsTr("开始日期:")
                                    font.pixelSize: 16
                                    color: ThemeManager.textColor
                                    verticalAlignment: Text.AlignVCenter
                                }

                                CustomButton {
                                    id: drawerStartDateField
                                    text: {
                                        if (selectedTodo && selectedTodo.recurrenceStartDate && !isNaN(selectedTodo.recurrenceStartDate.getTime()))
                                            return Qt.formatDate(selectedTodo.recurrenceStartDate, "yyyy-MM-dd");
                                        return qsTr("点击选择日期");
                                    }
                                    enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"
                                    font.pixelSize: 12
                                    Layout.fillWidth: true
                                    implicitHeight: 40
                                    is2ndColor: true

                                    onClicked: {
                                        startDatePicker.selectedDate = selectedTodo && selectedTodo.recurrenceStartDate ? selectedTodo.recurrenceStartDate : new Date();
                                        startDatePicker.open();
                                    }
                                }
                            }

                            // 完成状态
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                spacing: 8

                                IconButton {
                                    text: "\ue8eb"
                                }

                                CustomCheckBox {
                                    id: drawerCompletedCheckBox
                                    text: qsTr("已完成:")
                                    checked: selectedTodo && selectedTodo.completed !== undefined ? selectedTodo.completed : false
                                    enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle"
                                    fontSize: 16
                                    implicitHeight: 30
                                    isCheckBoxOnLeft: false

                                    onCheckedChanged: {
                                        if (selectedTodo && checked !== selectedTodo.completed) {
                                            todoManager.updateTodo(selectedTodo.index, "isCompleted", checked);
                                            selectedTodo.completed = checked;
                                        }
                                    }
                                }
                            }

                            // 重要程度
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                spacing: 8

                                IconButton {
                                    text: "\ue8de"
                                }

                                CustomCheckBox {
                                    id: drawerImportantCheckBox
                                    text: qsTr("重要:")
                                    checked: selectedTodo && selectedTodo.important !== undefined ? selectedTodo.important : false
                                    enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"
                                    fontSize: 16
                                    implicitHeight: 30
                                    isCheckBoxOnLeft: false

                                    onCheckedChanged: {
                                        if (selectedTodo && checked !== selectedTodo.important) {
                                            todoManager.updateTodo(selectedTodo.index, "important", checked);
                                            selectedTodo.important = checked;
                                        }
                                    }
                                }
                            }

                            // 删除按钮
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                spacing: 8

                                IconButton {
                                    text: "\ue8f5"
                                }

                                Text {
                                    text: qsTr("删除:")
                                    font.pixelSize: 16
                                    color: ThemeManager.textColor
                                    verticalAlignment: Text.AlignVCenter
                                }

                                CustomButton {
                                    text: qsTr("删除")
                                    font.pixelSize: 16
                                    enabled: selectedTodo !== null && todoFilter.currentFilter !== "recycle" && todoFilter.currentFilter !== "done"
                                    backgroundColor: ThemeManager.errorColor
                                    hoverColor: Qt.darker(ThemeManager.errorColor, 1.1)
                                    pressedColor: Qt.darker(ThemeManager.errorColor, 1.2)
                                    textColor: "white"
                                    implicitHeight: 40

                                    onClicked: {
                                        if (selectedTodo) {
                                            todoManager.removeTodo(selectedTodo.index);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 筛选菜单（从筛选按钮点击弹出）
    Menu {
        id: filterMenu
        width: 200
        height: implicitHeight
        z: 10000  // 确保菜单显示在最上层

        background: MainBackground {}

        // 排序选项
        MenuItem {
            text: qsTr("排序")
            enabled: false
            contentItem: Text {
                text: parent.text
                color: ThemeManager.textColor
                font.bold: true
                font.pixelSize: 14
            }
        }

        MenuItem {
            text: qsTr("按创建时间")
            onTriggered: todoSorter.setSortType(0)
            contentItem: Text {
                text: parent.text
                color: ThemeManager.textColor
                font.pixelSize: 12
            }
        }

        MenuItem {
            text: qsTr("按截止日期")
            onTriggered: todoSorter.setSortType(1)
            contentItem: Text {
                text: parent.text
                color: ThemeManager.textColor
                font.pixelSize: 12
            }
        }

        MenuItem {
            text: qsTr("按重要性")
            onTriggered: todoSorter.setSortType(2)
            contentItem: Text {
                text: parent.text
                color: ThemeManager.textColor
                font.pixelSize: 12
            }
        }

        MenuItem {
            text: qsTr("按标题")
            onTriggered: todoSorter.setSortType(3)
            contentItem: Text {
                text: parent.text
                color: ThemeManager.textColor
                font.pixelSize: 12
            }
        }
    }

    // 顶部用户菜单（从头像处点击弹出）
    Menu {
        id: userMenu
        width: 200
        height: implicitHeight
        z: 10000  // 确保菜单显示在最上层

        background: MainBackground {}

        MenuItem {
            text: userAuth.isLoggedIn ? qsTr("退出登录") : qsTr("登录")
            contentItem: Row {
                spacing: 8
                Text {
                    text: "\ue981"
                    font.family: "iconFont"
                    color: ThemeManager.textColor
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: parent.parent.text
                    color: ThemeManager.textColor
                    font.pixelSize: 14
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            onTriggered: {
                if (userAuth.isLoggedIn) {
                    loginStatusDialogs.showLogoutConfirm();
                } else {
                    loginStatusDialogs.showLoginDialog();
                }
            }
        }

        MenuItem {
            id: onlineToggleItem
            contentItem: RowLayout {
                spacing: 12
                ControlRow {
                    spacing: 8
                    icon: "\ue8ef"
                    text: qsTr("自动同步")
                    Layout.alignment: Qt.AlignVCenter // 上下居中
                    checked: todoSyncServer.isAutoSyncEnabled
                    controlType: ControlRow.ControlType.Switch

                    onCheckedChanged: {
                        if (checked) {
                            // 如果未登录，显示提示并重置开关
                            if (!todoManager.isLoggedIn) {
                                toggle();
                                homePage.loginStatusDialogs.showLoginRequired();
                            } else {
                                todoSyncServer.setAutoSyncEnabled(checked);
                            }
                        }
                    }
                }
            }
            // 阻止点击整行触发默认切换
            onTriggered: {}
        }
    }

    // 确认删除弹窗
    ModalDialog {
        id: confirmDeleteDialog
        property var selectedIndices: []

        dialogTitle: qsTr("确认删除")
        dialogWidth: 350
        message: qsTr("确定要永久删除选中的 %1 个待办事项吗？\n此操作无法撤销。").arg(confirmDeleteDialog.selectedIndices.length)

        // 执行硬删除
        onConfirmed: {
            var sortedIndices = confirmDeleteDialog.selectedIndices.slice().sort(function (a, b) {
                return b - a;
            });
            for (var i = 0; i < sortedIndices.length; i++) {
                todoManager.permanentlyDeleteTodo(sortedIndices[i]);
            }
            multiSelectMode = false;
            selectedItems = [];
            confirmDeleteDialog.close();
        }
    }

    // 截止日期选择器
    DateTimePicker {
        id: deadlineDatePicker

        onConfirmed: {
            if (selectedTodo) {
                todoManager.updateTodo(selectedTodo.index, "deadline", selectedDate);
                selectedTodo.deadline = selectedDate;
            }
        }
    }

    // 开始日期选择器
    DateTimePicker {
        id: startDatePicker
        enableTimeMode: false

        onConfirmed: {
            if (selectedTodo) {
                todoManager.updateTodo(selectedTodo.index, "recurrenceStartDate", selectedDate);
                selectedTodo.recurrenceStartDate = selectedDate;
            }
        }
    }
}
