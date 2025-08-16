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
        function onWidthChanged(width) {
            root.width = width;
        }
        function onHeightChanged(height) {
            root.height = height;
        }
    }

    // 标题栏
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        width: isDesktopWidget ? 400 : parent.width
        height: isDesktopWidget ? 35 : 45
        color: primaryColor
        border.color: isDesktopWidget ? borderColor : "transparent"
        border.width: isDesktopWidget ? 1 : 0
        radius: isDesktopWidget ? 5 : 0

        // 允许按住标题栏拖动窗口
        MouseArea {
            anchors.fill: parent
            property point clickPos: "0,0"

            onPressed: function(mouse) {
                clickPos = Qt.point(mouse.x, mouse.y);
            }

            onPositionChanged: function(mouse) {
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
            anchors.leftMargin: isDesktopWidget ? 5 : 10
            anchors.rightMargin: isDesktopWidget ? 5 : 10

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

            // 小组件模式按钮组
            RowLayout {
                Layout.fillWidth: isDesktopWidget ? true : false
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                spacing: 2
                visible: isDesktopWidget

                Button {
                    text: "☰"
                    onClicked: mainWindow.toggleSettingsVisible()
                    flat: true
                    implicitWidth: 30
                    implicitHeight: 30
                    background: Rectangle { color: "transparent" }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                Button {
                    text: isShowTodos ? "^" : "v"
                    onClicked: mainWindow.toggleTodosVisible()
                    flat: true
                    implicitWidth: 30
                    implicitHeight: 30
                    background: Rectangle { color: "transparent" }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                Button {
                    text: "+"
                    onClicked: mainWindow.toggleAddTaskVisible()
                    flat: true
                    implicitWidth: 30
                    implicitHeight: 30
                    background: Rectangle { color: "transparent" }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                Button {
                    text: isDesktopWidget ? "大" : "小"
                    onClicked: {
                        if (isDesktopWidget) {
                            mainWindow.toggleWidgetMode();
                            mainWindow.isShowTodos = true;
                        } else {
                            mainWindow.toggleWidgetMode();
                        }
                    }
                    flat: true
                    implicitWidth: 30
                    implicitHeight: 30
                    background: Rectangle { color: "transparent" }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }

            // 非小组件模式按钮组
            RowLayout {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                spacing: 5
                visible: !isDesktopWidget

                Button {
                    text: isDesktopWidget ? "大" : "小"
                    onClicked: {
                        if (isDesktopWidget) {
                            mainWindow.toggleWidgetMode();
                            mainWindow.isShowTodos = true;
                        } else {
                            mainWindow.toggleWidgetMode();
                        }
                    }
                    flat: true
                    implicitWidth: 30
                    background: Rectangle { color: "transparent" }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                // 关闭按钮
                Button {
                    text: "✕"
                    onClicked: root.close()
                    flat: true
                    implicitWidth: 30
                    background: Rectangle { color: "transparent" }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
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
        visible: !isDesktopWidget && depth > 0 // 小窗口模式时隐藏主页面
        initialItem: mainPageComponent
    }

    // 小组件模式组件
    WidgetMode {
        id: widgetModeComponent
        anchors.fill: parent
        isDesktopWidget: root.isDesktopWidget
        isShowSetting: root.isShowSetting
        isShowAddTask: root.isShowAddTask
        isShowTodos: root.isShowTodos
        isDarkMode: root.isDarkMode
        preventDragging: root.preventDragging
        visible: isDesktopWidget
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
            id: darkModeItem
            contentItem: RowLayout {
                spacing: 12
                Label {
                    text: qsTr("深色模式")
                    color: root.textColor
                }
                Switch {
                    id: darkModeSwitch
                    checked: root.isDarkMode
                    onCheckedChanged: root.isDarkMode = checked
                }
            }
        }

        MenuItem {
            id: onlineToggleItem
            contentItem: RowLayout {
                spacing: 12
                Label {
                    text: qsTr("自动同步")
                    color: root.textColor
                }
                Switch {
                    id: onlineSwitch
                    checked: todoModel.isOnline
                    onCheckedChanged: todoModel.isOnline = checked
                }
            }
            // 阻止点击整行触发默认切换
            onTriggered: {}
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
