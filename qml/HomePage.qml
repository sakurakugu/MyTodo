import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: homePage

    property var root
    property var stackView
    property var selectedTodo: null  // 当前选中的待办事项

    // 主布局
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧功能区
        Rectangle {
            Layout.preferredWidth: 60
            Layout.fillHeight: true
            border.width: 1

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
                            color: theme.secondaryBackgroundColor    ///< 使用主题次要背景色
                            Layout.alignment: Qt.AlignVCenter        ///< 垂直居中对齐

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

                // 各种筛选分类
                IconButton {
                    id: filterButton
                    text: "\ue90f"              ///< 筛选图标
                    onClicked: {
                        // 计算菜单位置，固定在筛选按钮右侧
                        var pos = mapToItem(null, width, height);
                        filterMenu.popup(pos.x, pos.y);
                    }
                    textColor: theme.textColor
                    fontSize: 16
                    isDarkMode: globalState.isDarkMode
                }

                // 标题栏剩余空间填充
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                // 刷新
                IconButton {
                    id: refreshButton
                    text: "\ue8ef"              ///< 刷新图标

                    // 添加旋转动画
                    RotationAnimation {
                        id: rotationAnimation
                        target: refreshButton
                        from: 0
                        to: 360
                        duration: 1000
                        running: false
                    }

                    onClicked: {
                        rotationAnimation.start();  // 开始旋转动画
                        todoManager.syncWithServer();
                    }

                    textColor: theme.textColor
                    fontSize: 16
                    isDarkMode: globalState.isDarkMode
                }

                // 深色模式
                IconButton {
                    text: globalState.isDarkMode ? "\ue668" : "\ue62e"
                    onClicked: {
                        globalState.isDarkMode = !globalState.isDarkMode;
                        setting.save("setting/isDarkMode", globalState.isDarkMode);
                    }
                    textColor: theme.textColor
                    fontSize: 16
                    isDarkMode: globalState.isDarkMode
                }

                // 设置
                IconButton {
                    text: "\ue913"              ///< 设置图标
                    onClicked: stackView.push(Qt.resolvedUrl("SettingPage.qml"), {
                        root: root,
                        stackView: stackView
                    })
                    textColor: theme.textColor
                    fontSize: 16
                    isDarkMode: globalState.isDarkMode
                }
            }
        }

        // 中间待办事项列表区域
        Rectangle {
            Layout.preferredWidth: 210
            Layout.fillHeight: true
            color: "white"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 搜索栏和工具栏
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    border.width: 1

                    // 窗口拖拽处理区域
                    WindowDragHandler {
                        anchors.fill: parent
                        targetWindow: root
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8

                        // 搜索框
                        TextField {
                            id: searchField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            placeholderText: "搜索"
                            selectByMouse: true
                            onTextChanged: {
                                todoFilter.setSearchText(text);
                            }
                        }

                        IconButton {
                            text: "?"
                            textColor: theme.textColor
                            fontSize: 16
                            isDarkMode: globalState.isDarkMode
                        }
                    }
                }

                // 待办事项列表
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
                                // 显示详情
                                selectedTodo = {
                                    title: model.title,
                                    description: model.description,
                                    category: model.category,
                                    important: model.important
                                };
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
                                color: model.isCompleted ? theme.completedColor : model.important ? theme.highImportantColor : theme.lowImportantColor

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
                                font.strikeout: model.isCompleted
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
        }

        // 右侧详情区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 窗口标题栏
                Rectangle {
                    id: titleBar
                    Layout.preferredHeight: 30
                    Layout.fillWidth: true
                    
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
                            textColor: theme.textColor
                            fontSize: 18
                            isDarkMode: globalState.isDarkMode
                        }

                        // 最小化按钮
                        IconButton {
                            text: "\ue65a"
                            onClicked: homePage.showMinimized()
                            textColor: theme.textColor
                            fontSize: 16
                            isDarkMode: globalState.isDarkMode
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
                            textColor: theme.textColor
                            fontSize: 16
                            isDarkMode: globalState.isDarkMode
                        }

                        // 关闭按钮
                        IconButton {
                            text: "\ue8d1"
                            onClicked: root.close()
                            fontSize: 16
                            textColor: theme.textColor
                            isDarkMode: globalState.isDarkMode
                        }
                    }
                }

                // 详情标题栏
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    // 窗口拖拽处理区域
                    WindowDragHandler {
                        anchors.fill: parent
                        targetWindow: root
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 16

                        Text {
                            text: "我是标题"
                            font.pixelSize: 18
                            font.bold: true
                            // color: "white"
                            Layout.fillWidth: true
                        }

                        IconButton {
                            text: "\ue90f"                      ///< 菜单图标
                            // TODO：点击弹出详情相关的设置菜单
                            textColor: theme.textColor
                            fontSize: 16
                            isDarkMode: globalState.isDarkMode
                        }
                    }

                    // 底部边框
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: "#000000" // 边框颜色
                    }
                }

                // 详情内容
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        anchors.margins: 16
                        spacing: 16

                        // 当有选中项目时显示详情
                        ColumnLayout {
                            visible: todoListView.currentIndex >= 0
                            Layout.fillWidth: true
                            spacing: 12

                            Text {
                                text: "标题"
                                font.pixelSize: 12
                                font.bold: true
                            }

                            Text {
                                text: todoListView.currentItem ? (todoListView.model.data(todoListView.model.index(todoListView.currentIndex, 0), todoManager.TitleRole) || "无标题") : ""
                                font.pixelSize: 16
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                            }

                            Text {
                                text: "描述"
                                font.pixelSize: 12
                                font.bold: true
                            }

                            Text {
                                text: todoListView.currentItem ? (todoListView.model.data(todoListView.model.index(todoListView.currentIndex, 0), todoManager.DescriptionRole) || "无描述") : ""
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                // 添加待办事项区域
                // TODO：里面是文本框，有按钮可以将其高度拉伸，覆盖详情内容
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150

                    // 顶部边框
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#000000" // 边框颜色
                    }

                    // 待办事项输入框
                    TextField {
                        id: newTodoField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100 // TODO:让他不超过其父组件150的高度
                        placeholderText: qsTr("输入待办事项")
                        selectByMouse: true
                        color: theme.textColor
                        // TODO:在下面的按钮添加一个选择,可以选择ctrl+enter还是enter触发,和qq一样
                        // TODO:还有 Key_Enter 和 Key_Return
                        onAccepted: {
                            if (text.trim() !== "") {
                                todoManager.addTodo(text.trim());
                                text = "";
                            }
                        }
                    }

                    RowLayout {
                        Item {
                            Layout.fillWidth: true
                        }
                        // 添加待办事项按钮
                        Button {
                            text: qsTr("添加")
                            onClicked: {
                                if (newTodoField.text.trim() !== "") {
                                    todoManager.addTodo(newTodoField.text.trim());
                                    newTodoField.text = "";
                                }
                            }

                            background: Rectangle {
                                color: parent.pressed ? (globalState.isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (globalState.isDarkMode ? "#3c5a78" : "#e0e0e0") : (globalState.isDarkMode ? "#2c3e50" : "#f0f0f0")
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

        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 4
        }

        // 排序选项
        MenuItem {
            text: qsTr("排序")
            enabled: false
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.bold: true
                font.pixelSize: 14
            }
        }

        MenuItem {
            text: qsTr("按创建时间")
            onTriggered: todoSorter.setSortType(0)
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
            }
        }

        MenuItem {
            text: qsTr("按截止日期")
            onTriggered: todoSorter.setSortType(1)
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
            }
        }

        MenuItem {
            text: qsTr("按重要性")
            onTriggered: todoSorter.setSortType(2)
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
            }
        }

        MenuItem {
            text: qsTr("按标题")
            onTriggered: todoSorter.setSortType(3)
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
            }
        }

        MenuSeparator {
            contentItem: Rectangle {
                implicitHeight: 1
                color: theme.borderColor
            }
        }

        // 倒序选项
        MenuItem {
            id: descendingMenuItem
            contentItem: RowLayout {
                spacing: 12
                Text {
                    text: qsTr("倒序排列")
                    color: theme.textColor
                    font.pixelSize: 12
                }
                Switch {
                    id: descendingSwitch
                    leftPadding: 0
                    scale: 0.7
                    onCheckedChanged: {
                        todoSorter.setDescending(checked);
                    }
                }
            }
            onTriggered: {}
        }

        MenuSeparator {
            contentItem: Rectangle {
                implicitHeight: 1
                color: theme.borderColor
            }
        }

        // 状态筛选
        MenuItem {
            text: qsTr("状态筛选")
            enabled: false
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.bold: true
                font.pixelSize: 14
            }
        }

        MenuItem {
            text: qsTr("全部")
            onTriggered: todoFilter.currentFilter = "all"
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
            }
        }

        MenuItem {
            text: qsTr("待办")
            onTriggered: todoFilter.currentFilter = "todo"
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
            }
        }

        MenuItem {
            text: qsTr("完成")
            onTriggered: todoFilter.currentFilter = "done"
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
            }
        }

        MenuItem {
            text: qsTr("回收站")
            onTriggered: todoFilter.currentFilter = "recycle"
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
            }
        }

        MenuSeparator {
            contentItem: Rectangle {
                implicitHeight: 1
                color: theme.borderColor
            }
        }

        // 分类筛选
        MenuItem {
            text: qsTr("分类筛选")
            enabled: false
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.bold: true
                font.pixelSize: 14
            }
        }

        MenuItem {
            text: qsTr("全部分类")
            onTriggered: todoFilter.currentCategory = ""
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
            }
        }

        // 动态分类菜单项将通过Repeater添加
        Repeater {
            model: categoryManager.categories
            MenuItem {
                text: modelData
                onTriggered: todoFilter.currentCategory = modelData
                contentItem: Text {
                    text: parent.text
                    color: theme.textColor
                    font.pixelSize: 12
                }
            }
        }
    }

    // 顶部用户菜单（从头像处点击弹出）
    Menu {
        id: userMenu
        width: 200
        height: implicitHeight
        z: 10000  // 确保菜单显示在最上层

        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 4
        }

        MenuItem {
            text: userAuth.isLoggedIn ? qsTr("退出登录") : qsTr("登录")
            contentItem: Row {
                spacing: 8
                Text {
                    text: "\ue981"
                    font.family: iconFont.name
                    color: theme.textColor
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: parent.parent.text
                    color: theme.textColor
                    // font: parent.parent.font
                    font.pixelSize: 18
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
                Row {
                    spacing: 8
                    Text {
                        text: "\ue8ef"
                        font.family: iconFont.name
                        color: theme.textColor
                        font.pixelSize: 18
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: qsTr("自动同步")
                        color: theme.textColor
                        font.pixelSize: 18
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Switch {
                    id: onlineSwitch
                    leftPadding: 0
                    checked: todoSyncServer.isAutoSyncEnabled
                    scale: 0.7

                    onCheckedChanged: {
                        if (checked && !userAuth.isLoggedIn) {
                            // 如果要开启自动同步但未登录，显示提示并重置开关
                            onlineSwitch.checked = false;
                            userMenu.close(); // 关闭菜单
                            loginStatusDialogs.showLoginRequired();
                        } else {
                            todoSyncServer.setAutoSyncEnabled(checked);
                        }
                    }
                }
            }
            // 阻止点击整行触发默认切换
            onTriggered: {}
        }
    }

    // 登录相关对话框组件
    LoginStatusDialogs {
        id: loginStatusDialogs
    }
}
