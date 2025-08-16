import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: mainPage
    
    // 属性绑定外部上下文
    property bool isDarkMode: false
    property bool isDesktopWidget: false
    // 提供根窗口引用，便于在子页面中读写全局属性
    property var rootWindow: null
    property color primaryColor: isDarkMode ? "#2c3e50" : "#4a86e8"
    property color backgroundColor: isDarkMode ? "#1e272e" : "white"
    property color secondaryBackgroundColor: isDarkMode ? "#2d3436" : "#f5f5f5"
    property color textColor: isDarkMode ? "#ecf0f1" : "black"
    property color borderColor: isDarkMode ? "#34495e" : "#cccccc"

    background: Rectangle {
        color: backgroundColor
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
                color: secondaryBackgroundColor
                visible: !isDesktopWidget

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    Label {
                        text: qsTr("分类")
                        font.bold: true
                        font.pixelSize: 16
                        color: textColor
                    }

                    Label {
                        text: "状态"
                        font.pixelSize: 14
                        color: textColor
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
                        color: textColor
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
                        color: textColor
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
                                width: 20; height: 20
                            }
                            Label {
                                text: todoListView.refreshing ? qsTr("正在同步...") : (todoListView.pullDistance >= todoListView.pullThreshold ? qsTr("释放刷新") : qsTr("下拉刷新"))
                                color: textColor
                                font.pixelSize: 12
                            }
                        }
                    }

                    Connections {
                        target: todoModel
                        onSyncStarted: {
                            if (!todoListView.refreshing && todoListView.atYBeginning) {
                                todoListView.refreshing = true;
                            }
                        }
                        onSyncCompleted: function(success, errorMessage) {
                            todoListView.refreshing = false;
                            todoListView.contentY = 0;
                        }
                    }

                    delegate: Rectangle {
                        width: todoListView.width
                        height: 50
                        color: index % 2 === 0 ? secondaryBackgroundColor : backgroundColor

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
                                color: model.status === "done" ? "#4caf50" : model.urgency === "high" ? "#f44336" : model.urgency === "medium" ? "#ff9800" : "#8bc34a"

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
                                color: textColor
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
                                        // 禁用项目的MouseArea
                                        itemMouseArea.enabled = false;
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
                        onTriggered: itemMouseArea.enabled = true
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
            titleField.text = todo.title || "";
            descriptionField.text = todo.description || "";
            categoryCombo.currentIndex = Math.max(0, categoryCombo.model.indexOf(todo.category));
            urgencyCombo.currentIndex = Math.max(0, urgencyCombo.model.indexOf(todo.urgency));
            importanceCombo.currentIndex = Math.max(0, importanceCombo.model.indexOf(todo.importance));
            visible = true;
        }

        ScrollView {
            anchors.fill: parent
            clip: true

            ColumnLayout {
                width: todoDetailsDialog.width  // 减去滚动条宽度
                spacing: 10

                Label {
                    text: qsTr("标题")
                    color: textColor
                }
                TextField {
                    id: titleField
                    Layout.fillWidth: true
                    color: textColor
                }

                Label {
                    text: qsTr("描述")
                    color: textColor
                }
                TextArea {
                    id: descriptionField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    wrapMode: TextEdit.Wrap
                    color: textColor
                }

                Label {
                    text: qsTr("分类")
                    color: textColor
                }
                ComboBox {
                    id: categoryCombo
                    model: ["工作", "学习", "生活", "其他"]
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Label {
                            text: qsTr("紧急程度")
                            color: textColor
                        }
                        ComboBox {
                            id: urgencyCombo
                            model: ["高", "中", "低"]
                            Layout.fillWidth: true
                        }
                    }

                    ColumnLayout {
                        Label {
                            text: qsTr("重要程度")
                            color: textColor
                        }
                        ComboBox {
                            id: importanceCombo
                            model: ["高", "中", "低"]
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }

        onAccepted: {
            if (currentTodo && titleField.text.trim() !== "") {
                todoModel.updateTodo(todoListView.currentIndex, {
                    "title": titleField.text.trim(),
                    "description": descriptionField.text.trim(),
                    "category": categoryCombo.currentText,
                    "urgency": urgencyCombo.currentText,
                    "importance": importanceCombo.currentText
                });
            }
        }
    }
}