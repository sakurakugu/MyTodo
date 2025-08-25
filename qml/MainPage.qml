/**
 * @file MainPage.qml
 * @brief 主页面组件
 *
 * 该文件定义了应用程序的主页面组件，包含侧边栏、任务列表和任务详情等功能。
 * 它是应用程序的主要界面，用户可以在其中查看和管理待办事项。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @version 2025-08-22 18:51:19(UTC+8) 周五
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Page {
    id: mainPage

    // 属性绑定外部上下文
    property bool isDarkMode: false
    property bool isDesktopWidget: false

    property var rootWindow: null       // 提供根窗口引用，便于在子页面中读写全局属性
    property bool sidebarExpanded: true // 侧边栏展开/收起状态
    property bool showDetails: false    // 详情显示状态
    property var selectedTodo: null     // 当前选中的待办事项

    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: mainPage.isDarkMode
    }

    background: Rectangle {
        color: theme.backgroundColor
        bottomLeftRadius: 5
        bottomRightRadius: 5
    }

    // 主内容区域，列式布局
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // 侧边栏和主内容区和详情显示区的分割
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // 左侧侧边栏
            Rectangle {
                id: sidebar
                Layout.preferredWidth: mainPage.sidebarExpanded ? 180 : 0
                Layout.fillHeight: true
                color: theme.secondaryBackgroundColor
                visible: !mainPage.isDesktopWidget
                radius: 5

                // 侧边栏宽度变化动画
                Behavior on Layout.preferredWidth {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutCubic
                    }
                }

                // 侧边栏标题部分
                RowLayout {
                    // 侧边栏标题
                    visible: mainPage.sidebarExpanded
                    Layout.fillWidth: true
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 10
                    spacing: 10
                    Label {
                        text: qsTr("筛选")
                        font.bold: true
                        font.pixelSize: 16
                        color: theme.textColor
                        Layout.alignment: Qt.AlignLeft
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    // 收起按钮
                    Button {
                        Layout.preferredWidth: 50
                        z: 10

                        background: Rectangle {
                            color: parent.pressed ? (mainPage.isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (mainPage.isDarkMode ? "#3c5a78" : "#e0e0e0") : (isDarkMode ? "#2c3e50" : "#f0f0f0")
                            border.color: theme.borderColor
                            border.width: 1
                            radius: 4
                        }

                        contentItem: Text {
                            text: "收起 ◀"
                            color: theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 12
                        }

                        onClicked: {
                            sidebarExpanded = !sidebarExpanded;
                        }
                    }
                }

                // 侧边栏内容区域
                ScrollView {
                    anchors.top: parent.children[0].bottom  // 紧贴标题区域底部
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: mainPage.sidebarExpanded ? 10 : 5
                    clip: true
                    visible: mainPage.sidebarExpanded

                    ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: parent.width
                        spacing: 10

                        Label {
                            text: "排序"
                            font.pixelSize: 14
                            color: theme.textColor
                        }

                        // 排序选择控件
                        ComboBox {
                            id: sortComboBox
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 36
                            model: ["按创建时间", "按截止日期", "按重要性", "按标题"]

                            background: Rectangle {
                                color: parent.pressed ? (isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (isDarkMode ? "#3c5a78" : "#e0e0e0") : (isDarkMode ? "#2c3e50" : "#f0f0f0")
                                border.color: theme.borderColor
                                border.width: 1
                                radius: 4
                            }

                            contentItem: Text {
                                text: sortComboBox.displayText
                                color: theme.textColor
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: 8
                            }

                            onCurrentIndexChanged: {
                                    todoSorter.setSortType(currentIndex);
                            }
                        }

                        // 倒序开关
                        Switch {
                            id: descendingSwitch
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            text: "倒序排列"
                            font.pixelSize: 12
                            onCheckedChanged: {
                                    todoSorter.setDescending(checked);
                            }
                        }

                        Label {
                            text: "状态"
                            font.pixelSize: 14
                            color: theme.textColor
                        }

                        ComboBox {
                            id: statusFilter
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            model: ["全部", "待办", "完成", "回收站"]
                            onCurrentTextChanged: {
                                if (currentText === "全部") {
                                    todoFilter.currentFilter = "";
                                } else if (currentText === "待办") {
                                    todoFilter.currentFilter = "todo";
                                } else if (currentText === "完成") {
                                    todoFilter.currentFilter = "done";
                                } else if (currentText === "回收站") {
                                    todoFilter.currentFilter = "recycle";
                                }
                            }
                        }

                        Label {
                            text: "分类"
                            font.pixelSize: 14
                            color: theme.textColor
                        }

                        ComboBox {
                            id: categoryFilter
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            model: categoryManager.categories
                            onCurrentTextChanged: {
                                if (currentText === "全部") {
                                    categoryManager.currentCategory = "";
                                } else {
                                    categoryManager.currentCategory = currentText;
                                }
                            }
                        }

                        // 日期筛选分隔线
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: theme.borderColor
                            Layout.topMargin: 10
                            Layout.bottomMargin: 10
                        }

                        Label {
                            text: "日期筛选"
                            font.pixelSize: 14
                            color: theme.textColor
                        }

                        CheckBox {
                            id: dateFilterEnabled
                            text: "启用日期筛选"
                            font.pixelSize: 12
                            checked: todoFilter.dateFilterEnabled
                            onCheckedChanged: {
                                todoFilter.dateFilterEnabled = checked;
                            }
                        }

                        Label {
                            text: "开始日期"
                            font.pixelSize: 12
                            color: theme.textColor
                            visible: dateFilterEnabled.checked
                        }

                        TextField {
                            id: startDateField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            placeholderText: "选择开始日期 (yyyy-MM-dd)"
                            visible: dateFilterEnabled.checked
                            text: todoFilter.dateFilterStart.getTime() > 0 ? Qt.formatDate(todoFilter.dateFilterStart, "yyyy-MM-dd") : ""
                            onTextChanged: {
                                if (text.length === 10) {
                                    var date = Date.fromLocaleString(Qt.locale(), text, "yyyy-MM-dd");
                                    if (!isNaN(date.getTime())) {
                                        todoFilter.dateFilterStart = date;
                                    }
                                } else if (text === "") {
                                    todoFilter.dateFilterStart = new Date(0); // 无效日期
                                }
                            }
                        }

                        Label {
                            text: "结束日期"
                            font.pixelSize: 12
                            color: theme.textColor
                            visible: dateFilterEnabled.checked
                        }

                        TextField {
                            id: endDateField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            placeholderText: "选择结束日期 (yyyy-MM-dd)"
                            visible: dateFilterEnabled.checked
                            text: todoFilter.dateFilterEnd.getTime() > 0 ? Qt.formatDate(todoFilter.dateFilterEnd, "yyyy-MM-dd") : ""
                            onTextChanged: {
                                if (text.length === 10) {
                                    var date = Date.fromLocaleString(Qt.locale(), text, "yyyy-MM-dd");
                                    if (!isNaN(date.getTime())) {
                                        todoFilter.dateFilterEnd = date;
                                    }
                                } else if (text === "") {
                                    todoFilter.dateFilterEnd = new Date(0); // 无效日期
                                }
                            }
                        }

                        Button {
                            text: "清除日期筛选"
                            Layout.fillWidth: true
                            visible: dateFilterEnabled.checked
                            onClicked: {
                                startDateField.text = "";
                                endDateField.text = "";
                                todoFilter.dateFilterStart = new Date(0);
                                todoFilter.dateFilterEnd = new Date(0);
                            }
                        }
                    }
                }
            }

            // 主内容区
            ColumnLayout {
                Layout.preferredWidth: mainPage.showDetails ? 180 : parent.width - (mainPage.sidebarExpanded ? 180 : 0) - 20
                Layout.fillHeight: true
                Layout.margins: 10
                spacing: 10

                Behavior on Layout.preferredWidth {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutCubic
                    }
                }

                // 添加新待办的区域
                RowLayout {
                    Layout.fillWidth: true

                    // 展开按钮
                    Button {
                        Layout.preferredWidth: 50
                        z: 10
                        visible: !sidebarExpanded

                        background: Rectangle {
                            color: parent.pressed ? (isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (isDarkMode ? "#3c5a78" : "#e0e0e0") : (isDarkMode ? "#2c3e50" : "#f0f0f0")
                            border.color: theme.borderColor
                            border.width: 1
                            radius: 4
                        }

                        contentItem: Text {
                            text: "筛选 ▶"
                            color: theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 12
                        }

                        onClicked: {
                            sidebarExpanded = !sidebarExpanded;
                        }
                    }

                    TextField {
                        id: newTodoField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        placeholderText: qsTr("添加新待办...")
                        color: theme.textColor
                        onAccepted: {
                            if (text.trim() !== "") {
                                todoManager.addTodo(text.trim());
                                text = "";
                            }
                        }
                    }

                    Button {
                        text: qsTr("添加")
                        onClicked: {
                            if (newTodoField.text.trim() !== "") {
                                todoManager.addTodo(newTodoField.text.trim());
                                newTodoField.text = "";
                            }
                        }

                        background: Rectangle {
                            color: parent.pressed ? (isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (isDarkMode ? "#3c5a78" : "#e0e0e0") : (isDarkMode ? "#2c3e50" : "#f0f0f0")
                            border.color: theme.borderColor
                            border.width: 1
                            radius: 4
                        }

                        contentItem: Text {
                            text: parent.text
                            color: theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        text: qsTr("类别管理")
                        onClicked: {
                            categoryManagementDialog.open();
                        }

                        background: Rectangle {
                            color: parent.pressed ? (isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (isDarkMode ? "#3c5a78" : "#e0e0e0") : (isDarkMode ? "#2c3e50" : "#f0f0f0")
                            border.color: theme.borderColor
                            border.width: 1
                            radius: 4
                        }

                        contentItem: Text {
                            text: parent.text
                            color: theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                // 待办列表
                ListView {
                    id: todoListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    // 使用 C++ 的 TodoManager
                    model: todoManager

                    // 下拉刷新相关属性与逻辑
                    property bool refreshing: false
                    property int pullThreshold: 60
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
                        width: todoListView.width
                        height: todoListView.refreshing ? 50 : Math.min(50, todoListView.pullDistance)
                        visible: height > 0 || todoListView.refreshing
                        Column {
                            anchors.centerIn: parent
                            spacing: 6
                            BusyIndicator {
                                running: todoListView.refreshing
                                visible: todoListView.refreshing || todoListView.pullDistance > 0
                                width: 20
                                height: 20
                            }
                            Label {
                                text: todoListView.refreshing ? qsTr("正在同步...") : (todoListView.pullDistance >= todoListView.pullThreshold ? qsTr("释放刷新") : qsTr("下拉刷新"))
                                color: theme.textColor
                                font.pixelSize: 12
                            }
                        }
                    }

                    Connections {
                        target: todoManager
                        function onSyncStarted() {
                            if (!todoListView.refreshing && todoListView.atYBeginning) {
                                todoListView.refreshing = true;
                            }
                        }
                        function onSyncCompleted(success, errorMessage) {
                            todoListView.refreshing = false;
                            todoListView.contentY = 0;
                        }
                    }

                    delegate: Rectangle {
                        id: delegateItem
                        width: parent ? parent.width : 0
                        height: 50
                        color: index % 2 === 0 ? theme.secondaryBackgroundColor : theme.backgroundColor

                        property alias itemMouseArea: itemMouseArea

                        // 点击项目查看/编辑详情
                        MouseArea {
                            id: itemMouseArea
                            anchors.fill: parent
                            z: 0  // 确保这个MouseArea在底层
                            onClicked: {
                                // 收起侧边栏
                                sidebarExpanded = false;
                                // 显示详情
                                selectedTodo = {
                                    title: model.title,
                                    description: model.description,
                                    category: model.category,
                                    important: model.important
                                };
                                detailsTaskForm.setFormData(selectedTodo);
                                showDetails = true;
                                todoListView.currentIndex = index;
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8
                            z: 1  // 确保RowLayout在MouseArea之上

                            // 待办状态指示器
                            Rectangle {
                                width: 16
                                height: 16
                                radius: 8
                                color: model.status === "done" ? theme.completedColor : model.important ? theme.highImportantColor : theme.lowImportantColor

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
                                font.strikeout: model.status === "done"
                                color: theme.textColor
                                Layout.fillWidth: true
                            }

                            // 删除按钮 - 使用独立的Rectangle和MouseArea
                            Rectangle {
                                width: 30
                                height: 30
                                color: "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "×"
                                    font.pixelSize: 16
                                    color: deleteMouseArea.pressed ? "darkgray" : "gray"
                                }

                                MouseArea {
                                    id: deleteMouseArea
                                    anchors.fill: parent
                                    onClicked: {
                                        // 禁用当前项目的MouseArea
                                        delegateItem.itemMouseArea.enabled = false;
                                        // 设置当前项索引
                                        todoListView.currentIndex = index;
                                        // 删除待办
                                        todoManager.removeTodo(index);
                                        // 延迟重新启用项目的MouseArea
                                        // 这是为了防止事件传播到下面的MouseArea
                                        timer.start();
                                    }
                                }
                            }
                        }
                    }

                    // 用于延迟重新启用itemMouseArea的定时器
                    Timer {
                        id: timer
                        interval: 10
                        onTriggered: {
                            // 重新启用当前项的MouseArea
                            if (todoListView.currentItem && todoListView.currentItem.itemMouseArea) {
                                todoListView.currentItem.itemMouseArea.enabled = true;
                            }
                        }
                    }

                    // 将垂直滚动条附加到ListView本身
                    ScrollBar.vertical: ScrollBar {}
                }
            }

            // 详情显示区域
            Rectangle {
                Layout.preferredWidth: mainPage.showDetails ? 300 : 0
                Layout.fillHeight: true
                color: theme.secondaryBackgroundColor
                visible: mainPage.showDetails

                Behavior on Layout.preferredWidth {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutCubic
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    // 详情标题栏
                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("待办详情")
                            font.bold: true
                            font.pixelSize: 16
                            color: theme.textColor
                            Layout.fillWidth: true
                        }

                        Button {
                            text: "×"
                            width: 30
                            height: 30

                            background: Rectangle {
                                color: parent.pressed ? (isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (isDarkMode ? "#3c5a78" : "#e0e0e0") : "transparent"
                                border.color: theme.borderColor
                                border.width: 1
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                color: theme.textColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 16
                            }

                            onClicked: {
                                mainPage.showDetails = false;
                                mainPage.selectedTodo = null;
                            }
                        }
                    }

                    // 详情内容
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        TaskForm {
                            id: detailsTaskForm
                            width: parent.width
                            theme: mainPage.theme
                        }
                    }

                    // 操作按钮
                    RowLayout {
                        Layout.fillWidth: true

                        Button {
                            text: qsTr("保存")
                            Layout.fillWidth: true

                            background: Rectangle {
                                color: parent.pressed ? (isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (isDarkMode ? "#3c5a78" : "#e0e0e0") : (isDarkMode ? "#2c3e50" : "#f0f0f0")
                                border.color: theme.borderColor
                                border.width: 1
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                color: theme.textColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                if (selectedTodo && detailsTaskForm.isValid()) {
                                    var todoData = detailsTaskForm.getTodoData();
                                    todoManager.updateTodo(todoListView.currentIndex, todoData);
                                    showDetails = false;
                                    selectedTodo = null;
                                }
                            }
                        }

                        Button {
                            text: qsTr("取消")
                            Layout.fillWidth: true

                            background: Rectangle {
                                color: parent.pressed ? (isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (isDarkMode ? "#3c5a78" : "#e0e0e0") : (isDarkMode ? "#2c3e50" : "#f0f0f0")
                                border.color: theme.borderColor
                                border.width: 1
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                color: theme.textColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                showDetails = false;
                                selectedTodo = null;
                            }
                        }
                    }
                }
            }
        }
    }

    // 类别管理对话框
    CategoryManagementDialog {
        id: categoryManagementDialog
        themeManager.isDarkMode: theme.isDarkMode
    }
}
