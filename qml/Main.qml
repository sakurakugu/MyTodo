import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: root
    width: 640
    height: 480
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: qsTr("我的待办")
    color: "transparent" // 要始终保持背景透明，不然window下再次变透明会变黑
    flags: Qt.FramelessWindowHint | (isDesktopWidget ? Qt.Tool : Qt.Window) // Qt.FramelessWindowHint 要一直保持存在，不然windows会在没有它时背景变黑，而且变不回来......

    property int resizeBorderWidth: 5     // 定义边框调整大小的边距
    property bool isDesktopWidget: mainWindow.isDesktopWidget  // 是否是桌面小组件模式
    property bool isShowTodos: mainWindow.isShowTodos       // 是否展示所有任务，在小组件模式下
    property bool isShowAddTask: mainWindow.isShowAddTask    // 是否展示添加任务
    property bool isShowSetting: mainWindow.isShowSetting    // 是否展示设置
    // property bool isDarkMode: settings.get("isDarkMode", false)
    property bool isDarkMode: false       // 添加深色模式属性
    property bool preventDragging: settings.get("preventDragging", false) // 是否防止拖动

    // 使用StackView进行页面导航

    // 颜色主题
    property color primaryColor: isDarkMode ? "#2c3e50" : "#4a86e8"
    property color backgroundColor: isDarkMode ? "#1e272e" : "white"
    property color secondaryBackgroundColor: isDarkMode ? "#2d3436" : "#f5f5f5"
    property color textColor: isDarkMode ? "#ecf0f1" : "black"
    property color borderColor: isDarkMode ? "#34495e" : "#cccccc"

    // 监听mainWindow的宽度和高度变化
    Connections {
        target: mainWindow
        function onWidthChanged() {
            root.width = mainWindow.width;
        }
        function onHeightChanged() {
            root.height = mainWindow.height;
        }
    }

    // 小组件模式的设置弹出窗口
    Popup {
        id: settingsPopup
        y: 50
        width: 400
        height: 300

        // implicitHeight: Math.min(600, contentItem.implicitHeight)
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
                CheckBox {
                    id: darkModeCheckBox
                    text: "深色模式"
                    checked: isDarkMode
                    onCheckedChanged: {
                        isDarkMode = checked;
                        // 保存设置到配置文件
                        settings.save("isDarkMode", isDarkMode);
                    }
                }

                CheckBox {
                    id: preventDraggingCheckBox
                    text: "防止拖动窗口（小窗口模式）"
                    checked: preventDragging
                    enabled: isDesktopWidget
                    onCheckedChanged: {
                        preventDragging = checked;
                        // 保存设置到配置文件
                        settings.save("preventDragging", preventDragging);
                    }
                }

                CheckBox {
                    id: autoSyncCheckBox
                    text: "自动同步"
                    checked: false
                    onCheckedChanged:
                    // TODO: 绑定自动同步逻辑
                    {}
                }

                Item {
                    Layout.fillHeight: true
                } // 占位

                Button {
                    text: "关闭"
                    Layout.alignment: Qt.AlignRight
                    onClicked: settingsPopup.close()
                }
            }
        }
    }

    // 小组件模式的添加任务弹出窗口
    Popup {
        id: addTaskPopup
        y: settingsPopup.visible ? settingsPopup.y + settingsPopup.height + 6 : 50
        width: 400
        height: 200
        // implicitHeight: Math.min(600, contentItem.implicitHeight)
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
                }

                TextField {
                    id: newTaskField
                    Layout.fillWidth: true
                    placeholderText: "输入新任务..."
                    onAccepted: {
                        if (text.trim() !== "") {
                            todoModel.addTodo(text.trim());
                            text = "";
                            addTaskPopup.close();
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
                        text: "取消"
                        onClicked: addTaskPopup.close()
                    }

                    Button {
                        text: "添加"
                        onClicked: {
                            if (newTaskField.text.trim() !== "") {
                                todoModel.addTodo(newTaskField.text.trim());
                                newTaskField.text = "";
                                addTaskPopup.close();
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

        // 根据其他弹出窗口的可见性动态计算位置
        y: {
            if (addTaskPopup.visible) {
                return addTaskPopup.y + addTaskPopup.height + 6;
            } else if (settingsPopup.visible) {
                return settingsPopup.y + settingsPopup.height + 6;
            }
            return 50;
        }

        width: 400
        height: 200
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
                }

                // 待办列表
                ListView {
                    id: todoListPopupView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: todoModel

                    delegate: Rectangle {
                        width: popupTodoListView.width
                        height: 40
                        color: index % 2 === 0 ? secondaryBackgroundColor : backgroundColor

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 5

                            // 待办状态指示器
                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: model.status === "done" ? "#4caf50" : model.urgency === "high" ? "#f44336" : model.urgency === "medium" ? "#ff9800" : "#8bc34a"

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        todoModel.markAsDone(index);
                                        mouse.accepted = true;
                                    }
                                }
                            }

                            // 待办标题
                            Label {
                                text: model.title
                                font.strikeout: model.status === "done"
                                font.pixelSize: 12
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            // 删除按钮
                            Rectangle {
                                width: 20
                                height: 20
                                color: "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "×"
                                    font.pixelSize: 14
                                    color: deletePopupMouseArea.pressed ? "darkgray" : "gray"
                                }

                                MouseArea {
                                    id: deletePopupMouseArea
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

    // 标题栏
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        width: parent.width
        height: isDesktopWidget ? 40 : 45
        color: primaryColor
        border.color: isDesktopWidget ? borderColor : "transparent"
        border.width: isDesktopWidget ? 1 : 0
        radius: isDesktopWidget ? 5 : 0

        // 允许按住标题栏拖动窗口
        MouseArea {
            anchors.fill: parent
            property point clickPos: "0,0"

            onPressed: {
                clickPos = Qt.point(mouse.x, mouse.y);
            }

            onPositionChanged: {
                // 只有在非小组件模式或小组件模式但未启用防止拖动时才允许拖动
                if (pressed && (!isDesktopWidget || (isDesktopWidget && !preventDragging))) {
                    var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y);
                    root.x += delta.x;
                    root.y += delta.y;
                }
            }
            z: -1 // 确保此MouseArea在其他控件下层，不影响其他控件的点击事件
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10

            // 用户头像和信息
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10
                visible: !isDesktopWidget

                // 用户头像和信息区域 - 只有这部分点击时弹出菜单
                MouseArea {
                    id: userProfileMouseArea
                    Layout.preferredWidth: childrenRect.width
                    Layout.fillHeight: true
                    onClicked: {
                        // 计算菜单位置，固定在用户头像下方
                        var pos = mapToItem(null, 0, height);
                        topMenu.popup(pos.x, pos.y);
                    }

                    RowLayout {
                        spacing: 10

                        // 用户头像
                        Rectangle {
                            width: 30
                            height: 30
                            radius: 15
                            color: "lightgray"
                            Layout.alignment: Qt.AlignVCenter  // 垂直居中对齐

                            Text {
                                anchors.centerIn: parent
                                text: "👤"
                                font.pixelSize: 18
                            }
                        }

                        // 用户名
                        Text {
                            text: todoModel.getUsername() !== "" ? todoModel.getUsername() : "未登录"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                            Layout.alignment: Qt.AlignVCenter  // 垂直居中对齐
                            horizontalAlignment: Text.AlignLeft  // 水平左对齐
                        }
                    }
                }

                // 标题栏剩余空间
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            MouseArea {
                id: settingsMouseArea
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                onClicked: settingsPopup.visible ? settingsPopup.close() : settingsPopup.open()
                visible: isDesktopWidget

                Text {
                    anchors.centerIn: parent
                    text: "☰"
                    color: "white"
                    font.pixelSize: 20
                }
            }

            Button {
                text: isShowTodos ? "^" : "v"
                onClicked: mainWindow.toggleTodosVisible()
                visible: isDesktopWidget
                flat: true
                implicitWidth: 30
                background: Rectangle {
                    color: "transparent"
                }
            }

            Button {
                text: isDesktopWidget ? "小" : "大"
                onClicked: {
                    if (isDesktopWidget) {
                        mainWindow.toggleWidgetMode();
                        mainWindow.setIsShowTodos(true);
                    } else {
                        mainWindow.toggleWidgetMode();
                    }
                }
                flat: true
                implicitWidth: 30
                background: Rectangle {
                    color: "transparent"
                }
            }

            Button {
                text: "+"
                onClicked: addTaskPopup.visible ? addTaskPopup.close() : addTaskPopup.open()
                flat: true
                visible: isDesktopWidget
                implicitWidth: 30
                background: Rectangle {
                    color: "transparent"
                }
            }

            // 关闭按钮
            Button {
                text: "✕"
                onClicked: root.close()
                flat: true
                visible: !isDesktopWidget
                implicitWidth: 30
                background: Rectangle {
                    color: "transparent"
                }
            }
        }
    }

    // 边框调整大小区域
    // 左边框
    MouseArea {
        id: leftResizeArea
        width: resizeBorderWidth
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: !isDesktopWidget
        cursorShape: Qt.SizeHorCursor

        property string edge: "left"

        onPressed: {
            root.startSystemResize(Qt.LeftEdge);
        }
    }

    // 右边框
    MouseArea {
        id: rightResizeArea
        width: resizeBorderWidth
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: !isDesktopWidget
        cursorShape: Qt.SizeHorCursor

        property string edge: "right"

        onPressed: {
            root.startSystemResize(Qt.RightEdge);
        }
    }

    // 上边框
    MouseArea {
        id: topResizeArea
        height: resizeBorderWidth
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        visible: !isDesktopWidget
        cursorShape: Qt.SizeVerCursor

        property string edge: "top"

        onPressed: {
            root.startSystemResize(Qt.TopEdge);
        }
    }

    // 下边框
    MouseArea {
        id: bottomResizeArea
        height: resizeBorderWidth
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: !isDesktopWidget
        cursorShape: Qt.SizeVerCursor

        property string edge: "bottom"

        onPressed: {
            root.startSystemResize(Qt.BottomEdge);
        }
    }

    // 左上角
    MouseArea {
        id: topLeftResizeArea
        width: resizeBorderWidth
        height: resizeBorderWidth
        anchors.left: parent.left
        anchors.top: parent.top
        visible: !isDesktopWidget
        cursorShape: Qt.SizeFDiagCursor

        property string edge: "topleft"

        onPressed: {
            root.startSystemResize(Qt.TopEdge | Qt.LeftEdge);
        }
    }

    // 右上角
    MouseArea {
        id: topRightResizeArea
        width: resizeBorderWidth
        height: resizeBorderWidth
        anchors.right: parent.right
        anchors.top: parent.top
        visible: !isDesktopWidget
        cursorShape: Qt.SizeBDiagCursor

        property string edge: "topright"

        onPressed: {
            root.startSystemResize(Qt.TopEdge | Qt.RightEdge);
        }
    }

    // 左下角
    MouseArea {
        id: bottomLeftResizeArea
        width: resizeBorderWidth
        height: resizeBorderWidth
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        visible: !isDesktopWidget
        cursorShape: Qt.SizeBDiagCursor

        property string edge: "bottomleft"

        onPressed: {
            root.startSystemResize(Qt.BottomEdge | Qt.LeftEdge);
        }
    }

    // 右下角
    MouseArea {
        id: bottomRightResizeArea
        width: resizeBorderWidth
        height: resizeBorderWidth
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: !isDesktopWidget
        cursorShape: Qt.SizeFDiagCursor

        property string edge: "bottomright"

        onPressed: {
            root.startSystemResize(Qt.BottomEdge | Qt.RightEdge);
        }
    }

    // 导航栈，使用内置 Page 进行页面管理
    StackView {
        id: stackView
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        z: 1000
        focus: true
        visible: depth > 0 // 仅在有页面时显示
        initialItem: mainPageComponent
    }

    // 顶部用户菜单（从头像处点击弹出）
    Menu {
        id: topMenu

        MenuItem {
            text: qsTr("设置")
            onTriggered: {
                stackView.push(Qt.resolvedUrl("Setting.qml"), {
                    isDarkMode: root.isDarkMode,
                    rootWindow: root
                });
            }
        }

        MenuItem {
            text: root.isDarkMode ? qsTr("切换为浅色模式") : qsTr("切换为深色模式")
            onTriggered: root.isDarkMode = !root.isDarkMode
        }

        MenuItem {
            text: (todoModel.isOnline ? qsTr("自动同步: 开") : qsTr("自动同步: 关"))
            onTriggered: todoModel.isOnline = !todoModel.isOnline
        }
    }
    // 通用确认对话框组件
    Component {
        id: confirmDialogComponent

        Dialog {
            id: confirmDialog
            title: qsTr("确认")
            standardButtons: Dialog.NoButton
            modal: true

            property string message: ""
            property string yesButtonText: qsTr("确定")
            property string noButtonText: qsTr("取消")

            contentItem: ColumnLayout {
                spacing: 20

                Label {
                    text: confirmDialog.message
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 10

                    Button {
                        text: confirmDialog.noButtonText
                        onClicked: confirmDialog.reject()
                    }

                    Button {
                        text: confirmDialog.yesButtonText
                        onClicked: confirmDialog.accept()
                    }
                }
            }
        }
    }

    Component {
        id: mainPageComponent
        MainPage {
            isDarkMode: root.isDarkMode
            isDesktopWidget: root.isDesktopWidget
            rootWindow: root
        }
    }
}
