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
    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: mainPage.isDarkMode
    }

    background: Rectangle {
        color: theme.backgroundColor
    }

    // 主内容区域，使用ColumnLayout
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: isDesktopWidget ? 5 : 10
        spacing: 10

        // 侧边栏和主内容区的分割
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // 左侧侧边栏
            Rectangle {
                id: sidebar
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                color: theme.secondaryBackgroundColor
                visible: !isDesktopWidget

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
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
                        model: ["全部", "待办", "完成"]
                        onCurrentTextChanged: {
                            todoModel.currentFilter = (currentText === "全部" ? "" : currentText === "待办" ? "todo" : "done");
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
                        model: ["全部", "工作", "学习", "生活", "其他"]
                        onCurrentTextChanged: {
                            todoModel.currentCategory = (currentText === "全部" ? "" : currentText);
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    Button {
                        text: qsTr("同步")
                        Layout.fillWidth: true

                        background: Rectangle {
                            color: parent.pressed ? theme.buttonPressedColor : parent.hovered ? theme.buttonHoverColor : theme.buttonNormalColor
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
                            todoModel.syncWithServer();
                        }
                    }
                }
            }

            // 主内容区
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 10
                spacing: 10

                // 添加新待办的区域
                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        id: newTodoField
                        Layout.fillWidth: true
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
                            color: parent.pressed ? theme.buttonPressedColor : parent.hovered ? theme.buttonHoverColor : theme.buttonNormalColor
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
                        width: todoListView.width
                        height: 50
                        color: index % 2 === 0 ? theme.secondaryBackgroundColor : theme.backgroundColor

                        property alias itemMouseArea: itemMouseArea

                        // 点击项目查看/编辑详情
                        MouseArea {
                            id: itemMouseArea
                            anchors.fill: parent
                            z: 0  // 确保这个MouseArea在底层
                            onClicked: {
                                todoDetailsDialog.open({
                                    title: model.title,
                                    description: model.description,
                                    category: model.category,
                                    urgency: model.urgency,
                                    importance: model.importance
                                });
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
                                color: model.status === "done" ? theme.completedColor : model.urgency === "high" ? theme.highUrgencyColor : model.urgency === "medium" ? theme.mediumUrgencyColor : theme.lowUrgencyColor

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
        }
    }

    // 待办详情/编辑对话框
    Dialog {
        id: todoDetailsDialog
        title: qsTr("待办详情")
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: 400
        height: Math.min(parent.height * 0.8, 500)  // 设置最大高度

        property var currentTodo: null

        function open(todo) {
            currentTodo = todo;
            taskForm.setFormData(todo);
            visible = true;
        }

        ScrollView {
            anchors.fill: parent
            clip: true

            TaskForm {
                id: taskForm
                width: parent.width
                theme: mainPage.theme
            }
        }

        onAccepted: {
            if (currentTodo && taskForm.isValid()) {
                var todoData = taskForm.getTodoData();
                todoModel.updateTodo(todoListView.currentIndex, todoData);
            }
        }
    }
}
