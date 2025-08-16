import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: widgetMode
    
    // 从父组件传入的属性
    property bool isDesktopWidget: false
    property bool isShowSetting: false
    property bool isShowAddTask: false
    property bool isShowTodos: false
    property bool isDarkMode: false
    property bool preventDragging: false
    
    // 自定义信号定义
    signal preventDraggingToggled(bool value)
    signal darkModeToggled(bool value)
    
    // 颜色主题
    property color primaryColor: isDarkMode ? "#2c3e50" : "#4a86e8"
    property color backgroundColor: isDarkMode ? "#1e272e" : "white"
    property color secondaryBackgroundColor: isDarkMode ? "#2d3436" : "#f5f5f5"
    property color textColor: isDarkMode ? "#ecf0f1" : "black"
    property color borderColor: isDarkMode ? "#34495e" : "#cccccc"
    
    // 小组件模式的设置弹出窗口
    Popup {
        id: settingsPopup
        y: 50
        width: 400
        height: 250

        modal: false // 非模态，允许同时打开多个弹出窗口
        focus: true
        closePolicy: Popup.NoAutoClose
        visible: isDesktopWidget && isShowSetting

        contentItem: Rectangle {
            color: secondaryBackgroundColor
            border.color: borderColor
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
                    color: textColor
                }

                // 设置内容
                Switch {
                    id: darkModeCheckBox
                    text: "深色模式"
                    checked: isDarkMode
                    onCheckedChanged: {
                        // 保存设置到配置文件
                        settings.save("isDarkMode", checked);
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
                        settings.save("preventDragging", checked);
                        // 发出信号通知父组件
                        widgetMode.preventDraggingToggled(checked);
                    }
                }

                Switch {
                    id: autoSyncSwitch
                    text: "自动同步"
                    checked: todoModel.isOnline
                    onCheckedChanged: {
                        todoModel.isOnline = checked;
                        // 保存逻辑统一在C++ setIsOnline 中
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
            color: secondaryBackgroundColor
            border.color: borderColor
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
                    color: textColor
                }

                TextField {
                    id: newTaskField
                    Layout.fillWidth: true
                    placeholderText: "输入新任务..."
                    onAccepted: {
                        if (text.trim() !== "") {
                            todoModel.addTodo(text.trim());
                            text = "";
                            mainWindow.isShowAddTask = false;
                        }
                    }
                }

                ComboBox {
                    id: taskCategory
                    Layout.fillWidth: true
                    model: ["工作", "学习", "生活", "其他"]
                    currentIndex: 0
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight

                    Button {
                        text: "添加"
                        onClicked: {
                            if (newTaskField.text.trim() !== "") {
                                todoModel.addTodo(newTaskField.text.trim());
                                newTaskField.text = "";
                                mainWindow.isShowAddTask = false;
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
        y: {
            var maxY = widgetMode.parent ? widgetMode.parent.height - baseHeight - 10 : calculatedY;
            return Math.min(calculatedY, maxY);
        }

        width: 400
        height: {
            var availableHeight = widgetMode.parent ? widgetMode.parent.height - calculatedY - 60 : baseHeight;
            return Math.min(baseHeight, availableHeight);
        }
        modal: false // 非模态，允许同时打开多个弹出窗口
        focus: true
        closePolicy: Popup.NoAutoClose
        visible: isDesktopWidget && isShowTodos // 在小组件模式下且需要显示所有任务时显示

        contentItem: Rectangle {
            color: secondaryBackgroundColor
            border.color: borderColor
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
                    color: textColor
                }

                // 待办列表
                ListView {
                    id: todoListPopupView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: todoModel

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
                            todoModel.syncWithServer();
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
                                color: textColor
                                font.pixelSize: 11
                            }
                        }
                    }

                    Connections {
                        target: todoModel
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
                        color: index % 2 === 0 ? secondaryBackgroundColor : backgroundColor

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 5

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
                                color: textColor
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
                                        todoModel.removeTodo(index);
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
}