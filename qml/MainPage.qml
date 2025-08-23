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
    // 提供根窗口引用，便于在子页面中读写全局属性
    property var rootWindow: null
    // 侧边栏展开/收起状态
    property bool sidebarExpanded: true
    // 详情显示状态
    property bool showDetails: false
    // 当前选中的待办事项
    property var selectedTodo: null
    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: mainPage.isDarkMode
    }

    background: Rectangle {
        color: theme.backgroundColor
    }

    // 主内容区域，列式布局
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: mainPage.isDesktopWidget ? 5 : 10
        spacing: 10

        // 侧边栏和主内容区的分割
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // 左侧侧边栏
            Rectangle {
                id: sidebar
                Layout.preferredWidth: sidebarExpanded ? 200 : 40
                Layout.fillHeight: true
                color: theme.secondaryBackgroundColor
                visible: !isDesktopWidget

                Behavior on Layout.preferredWidth {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutCubic
                    }
                }

                // 收起/展开按钮
                Button {
                    id: toggleButton
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 5
                    width: 30
                    height: 30
                    z: 10

                    background: Rectangle {
                        color: parent.pressed ? (isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (isDarkMode ? "#3c5a78" : "#e0e0e0") : (isDarkMode ? "#2c3e50" : "#f0f0f0")
                        border.color: theme.borderColor
                        border.width: 1
                        radius: 4
                    }

                    contentItem: Text {
                        text: sidebarExpanded ? "◀" : "▶"
                        color: theme.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 12
                    }

                    onClicked: {
                        sidebarExpanded = !sidebarExpanded;
                    }
                }

                // 侧边栏内容区域
                ScrollView {
                    anchors.fill: parent
                    anchors.topMargin: 40  // 为切换按钮留出空间
                    anchors.margins: sidebarExpanded ? 10 : 5
                    clip: true
                    visible: sidebarExpanded

                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: parent.width
                        spacing: 10

                        Label {
                            text: qsTr("分类")
                            font.bold: true
                            font.pixelSize: 16
                            color: theme.textColor
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
                                    todoModel.currentFilter = "";
                                } else if (currentText === "待办") {
                                    todoModel.currentFilter = "todo";
                                } else if (currentText === "完成") {
                                    todoModel.currentFilter = "done";
                                } else if (currentText === "回收站") {
                                    todoModel.currentFilter = "recycle";
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
                            model: todoModel.categories
                            onCurrentTextChanged: {
                                if (currentText === "全部") {
                                    todoModel.currentCategory = "";
                                } else {
                                    todoModel.currentCategory = currentText;
                                }
                            }
                        }

                        Label {
                            text: "重要程度"
                            font.pixelSize: 14
                            color: theme.textColor
                        }

                        ComboBox {
                            id: importantFilter
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            model: ["全部", "重要", "普通"]
                            onCurrentTextChanged: {
                                if (currentText === "全部") {
                                    todoModel.currentFilter = ""; // 清除筛选
                                } else if (currentText === "重要") {
                                    todoModel.currentFilter = "important";
                                    todoModel.currentImportant = true;
                                } else {
                                    // "普通"
                                    todoModel.currentFilter = "important";
                                    todoModel.currentImportant = false;
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
                            checked: todoModel.dateFilterEnabled
                            onCheckedChanged: {
                                todoModel.dateFilterEnabled = checked;
                            }
                        }

                        Label {
                            text: "开始日期"
                            font.pixelSize: 12
                            color: theme.textColor
                            enabled: dateFilterEnabled.checked
                        }

                        TextField {
                            id: startDateField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            placeholderText: "选择开始日期 (yyyy-MM-dd)"
                            enabled: dateFilterEnabled.checked
                            text: todoModel.dateFilterStart.getTime() > 0 ? Qt.formatDate(todoModel.dateFilterStart, "yyyy-MM-dd") : ""
                            onTextChanged: {
                                if (text.length === 10) {
                                    var date = Date.fromLocaleString(Qt.locale(), text, "yyyy-MM-dd");
                                    if (!isNaN(date.getTime())) {
                                        todoModel.dateFilterStart = date;
                                    }
                                } else if (text === "") {
                                    todoModel.dateFilterStart = new Date(0); // 无效日期
                                }
                            }
                        }

                        Label {
                            text: "结束日期"
                            font.pixelSize: 12
                            color: theme.textColor
                            enabled: dateFilterEnabled.checked
                        }

                        TextField {
                            id: endDateField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            placeholderText: "选择结束日期 (yyyy-MM-dd)"
                            enabled: dateFilterEnabled.checked
                            text: todoModel.dateFilterEnd.getTime() > 0 ? Qt.formatDate(todoModel.dateFilterEnd, "yyyy-MM-dd") : ""
                            onTextChanged: {
                                if (text.length === 10) {
                                    var date = Date.fromLocaleString(Qt.locale(), text, "yyyy-MM-dd");
                                    if (!isNaN(date.getTime())) {
                                        todoModel.dateFilterEnd = date;
                                    }
                                } else if (text === "") {
                                    todoModel.dateFilterEnd = new Date(0); // 无效日期
                                }
                            }
                        }

                        Button {
                            text: "清除日期筛选"
                            Layout.fillWidth: true
                            enabled: dateFilterEnabled.checked
                            onClicked: {
                                startDateField.text = "";
                                endDateField.text = "";
                                todoModel.dateFilterStart = new Date(0);
                                todoModel.dateFilterEnd = new Date(0);
                            }
                        }
                    }
                }
            }

            // 主内容区
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // 待办列表区域
                ColumnLayout {
                    Layout.preferredWidth: showDetails ? parent.width * 0.6 : parent.width
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

                        TextField {
                            id: newTodoField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            placeholderText: qsTr("添加新待办...")
                            color: theme.textColor
                            onAccepted: {
                                if (text.trim() !== "") {
                                    todoModel.addTodo(text.trim());
                                    text = "";
                                }
                            }
                        }

                        Button {
                            text: qsTr("添加")
                            onClicked: {
                                if (newTodoField.text.trim() !== "") {
                                    todoModel.addTodo(newTodoField.text.trim());
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

                        // 使用 C++ 的 TodoModel
                        model: todoModel

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
                                todoModel.syncWithServer();
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
                            target: todoModel
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
                            width: parent.width
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
                                            todoModel.markAsDone(index);
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
                                            todoModel.removeTodo(index);
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
                    Layout.preferredWidth: showDetails ? parent.width * 0.4 : 0
                    Layout.fillHeight: true
                    color: theme.secondaryBackgroundColor
                    visible: showDetails

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
                                    showDetails = false;
                                    selectedTodo = null;
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
                                        todoModel.updateTodo(todoListView.currentIndex, todoData);
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
    }

    // 类别管理对话框
    CategoryManagementDialog {
        id: categoryManagementDialog
        themeManager.isDarkMode: theme.isDarkMode
    }
}
