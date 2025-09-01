import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: homePage

    property var root
    property var stackView
    property bool refreshing: false       // 是否正在刷新
    property var selectedTodo: null       // 当前选中的待办事项
    property bool multiSelectMode: false  // 多选模式
    property var selectedItems: []        // 选中的项目索引列表

    // 组件完成时设置默认过滤器为"all"
    Component.onCompleted: {
        todoFilter.currentFilter = "all";
    }

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

                // 全部、未完成、已完成、回收站
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: implicitHeight
                    implicitHeight: column.implicitHeight + 20  // Column高度 + 上下边距
                    color: theme.borderColor
                    radius: 50

                    Column {
                        id: column
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        IconButton {
                            text: "\ue8df"              ///< 全部图标
                            onClicked: todoFilter.currentFilter = "all"
                            textColor: theme.textColor
                            fontSize: 16
                            isDarkMode: globalState.isDarkMode
                            width: parent.width
                        }

                        IconButton {
                            text: "\ue7fc"              ///< 未完成图标
                            onClicked: todoFilter.currentFilter = "todo"
                            textColor: theme.textColor
                            fontSize: 16
                            isDarkMode: globalState.isDarkMode
                            width: parent.width
                        }

                        IconButton {
                            text: "\ue7fa"              ///< 已完成图标
                            onClicked: todoFilter.currentFilter = "done"
                            textColor: theme.textColor
                            fontSize: 16
                            isDarkMode: globalState.isDarkMode
                            width: parent.width
                        }

                        IconButton {
                            text: "\ue922"              ///< 回收站图标
                            onClicked: todoFilter.currentFilter = "recycle"
                            textColor: theme.textColor
                            fontSize: 16
                            isDarkMode: globalState.isDarkMode
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
                    textColor: theme.textColor
                    fontSize: 16
                    isDarkMode: globalState.isDarkMode
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
                    textColor: theme.textColor
                    fontSize: 16
                    isDarkMode: globalState.isDarkMode
                }

                //  种类筛选
                IconButton {
                    id: categoryFilterButton
                    Layout.alignment: Qt.AlignHCenter
                    text: "\ue90f"              ///< 筛选图标
                    onClicked: {
                        // 计算菜单位置，固定在筛选按钮右侧
                        var pos = mapToItem(null, width, height);
                        categoryFilterMenu.popup(pos.x, pos.y);
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
                        if (!refreshing) {
                            refreshing = true;
                            refreshButtonAnimation.start();  // 开始旋转动画
                            todoManager.syncWithServer();
                        }
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

                        // 搜索框容器
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            color: theme.backgroundColor
                            border.color: theme.borderColor
                            border.width: 1
                            radius: 4

                            // 搜索图标
                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: "\ue8f2"              ///< 查询图标
                                color: theme.textColor
                                font.pixelSize: 16
                                font.family: "iconfont"
                            }

                            // 搜索框
                            TextField {
                                id: searchField
                                anchors.fill: parent
                                anchors.leftMargin: 35  // 为图标留出空间
                                anchors.rightMargin: 10
                                placeholderText: "搜索"
                                selectByMouse: true
                                verticalAlignment: TextInput.AlignVCenter
                                background: Rectangle {
                                    color: "transparent"
                                }
                                onTextChanged: {
                                    todoFilter.searchText = text;
                                }
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
                    property int pullThreshold: 20
                    property real pullDistance: 0

                    onContentYChanged: {
                        pullDistance = contentY < 0 ? -contentY : 0;
                    }

                    onMovementEnded: {
                        // 如果下拉距离超过阈值且在顶部，触发刷新
                        if (contentY < -pullThreshold && atYBeginning && !refreshing) {
                            refreshing = true;
                            todoManager.syncWithServer();
                        } else {}
                    }

                    header: Item {
                        width: todoListView.width
                        height: homePage.refreshing ? 50 : Math.min(50, todoListView.pullDistance)
                        visible: height > 0 || homePage.refreshing
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            // 刷新
                            IconButton {
                                id: refreshIndicatorIcon
                                text: "\ue8e2"              // 刷新图标
                                visible: homePage.refreshing || todoListView.pullDistance > 0
                                width: 20
                                height: 20
                                textColor: theme.textColor
                                fontSize: 16
                                isDarkMode: globalState.isDarkMode
                                Layout.alignment: Qt.AlignVCenter

                                // 旋转动画
                                RotationAnimation {
                                    target: refreshIndicatorIcon
                                    from: 0
                                    to: 360
                                    duration: 1000
                                    loops: Animation.Infinite
                                    running: homePage.refreshing
                                }
                            }
                            Label {
                                text: homePage.refreshing ? qsTr("正在同步...") : (todoListView.pullDistance >= todoListView.pullThreshold ? qsTr("释放刷新") : qsTr("下拉刷新"))
                                color: theme.textColor
                                font.pixelSize: 12
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }

                    // 下拉距离重置动画
                    NumberAnimation {
                        id: pullDistanceAnimation
                        target: todoListView
                        property: "pullDistance"
                        to: 0
                        duration: 300
                        easing.type: Easing.OutCubic
                    }

                    Connections {
                        target: todoManager
                        function onSyncStarted() {
                            if (!homePage.refreshing && todoListView.atYBeginning) {
                                homePage.refreshing = true;
                            }
                        }
                        function onSyncCompleted(result, message) {
                            homePage.refreshing = false;
                            refreshButtonAnimation.stop();  // 停止刷新按钮的旋转动画
                            // 使用动画平滑重置下拉距离
                            pullDistanceAnimation.start();
                            // 强制刷新ListView以确保显示最新数据
                            todoListView.model = null;
                            todoListView.model = todoManager;
                        }
                    }

                    delegate: Item {
                        id: delegateItem
                        width: parent ? parent.width : 0
                        height: 50

                        property bool isSelected: selectedItems.indexOf(index) !== -1
                        property real swipeOffset: 0
                        property bool swipeActive: false

                        // 主内容区域
                        Rectangle {
                            id: mainContent
                            anchors.fill: parent
                            x: delegateItem.swipeOffset
                            color: delegateItem.isSelected ? theme.selectedColor || "lightblue" : (index % 2 === 0 ? theme.secondaryBackgroundColor : theme.backgroundColor)

                            // 长按和点击处理
                            MouseArea {
                                id: itemMouseArea
                                anchors.fill: parent

                                onClicked: {
                                    if (multiSelectMode) {
                                        // 多选模式下切换选中状态
                                        var newSelectedItems = selectedItems.slice();
                                        var itemIndex = newSelectedItems.indexOf(index);
                                        if (itemIndex !== -1) {
                                            newSelectedItems.splice(itemIndex, 1);
                                        } else {
                                            newSelectedItems.push(index);
                                        }
                                        selectedItems = newSelectedItems;

                                        // 如果没有选中项，退出多选模式
                                        if (selectedItems.length === 0) {
                                            multiSelectMode = false;
                                        }
                                    } else {
                                        // 普通模式下显示详情
                                        selectedTodo = {
                                            title: model.title,
                                            description: model.description,
                                            category: model.category,
                                            important: model.important
                                        };
                                        todoListView.currentIndex = index;
                                    }
                                }

                                onPressAndHold: {
                                    // 长按进入多选模式
                                    if (!multiSelectMode) {
                                        multiSelectMode = true;
                                        selectedItems = [index];
                                    }
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 8

                                // 待办状态指示器
                                Rectangle {
                                    width: 16
                                    height: 16
                                    radius: 8
                                    color: model.isCompleted ? theme.completedColor : model.important ? theme.highImportantColor : theme.lowImportantColor

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: function (mouse) {
                                            if (!multiSelectMode) {
                                                todoManager.markAsDone(index);
                                            }
                                            mouse.accepted = true;
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

                                // 多选复选框（仅在多选模式下显示）
                                CheckBox {
                                    visible: multiSelectMode
                                    checked: delegateItem.isSelected
                                    Layout.preferredWidth: visible ? implicitWidth : 0
                                    Layout.preferredHeight: visible ? implicitHeight : 0
                                    onClicked: {
                                        var newSelectedItems = selectedItems.slice();
                                        var itemIndex = newSelectedItems.indexOf(index);
                                        if (checked && itemIndex === -1) {
                                            newSelectedItems.push(index);
                                        } else if (!checked && itemIndex !== -1) {
                                            newSelectedItems.splice(itemIndex, 1);
                                        }
                                        selectedItems = newSelectedItems;

                                        // 如果没有选中项，退出多选模式
                                        if (selectedItems.length === 0) {
                                            multiSelectMode = false;
                                        }
                                    }
                                }
                            }
                        }

                        // 左滑删除背景
                        Rectangle {
                            id: deleteBackground
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: Math.abs(delegateItem.swipeOffset)
                            color: "red"
                            visible: delegateItem.swipeOffset < 0

                            Text {
                                anchors.centerIn: parent
                                text: "删除"
                                color: "white"
                                font.pixelSize: 14
                            }

                            MouseArea {
                                id: deleteMouseArea
                                anchors.fill: parent
                                z: 10  // 确保在最上层
                                hoverEnabled: true
                                onClicked: {
                                    itemMouseArea.enabled = false; // 先禁用以防多次点
                                    // 设置当前项索引
                                    todoListView.currentIndex = index;

                                    // 检查是否在回收站中
                                    if (todoFilter.currentFilter === "recycle") {
                                        // 在回收站中，弹出确认弹窗进行硬删除
                                        confirmDeleteDialog.selectedIndices = [index];
                                        confirmDeleteDialog.open();
                                    } else {
                                        // 不在回收站中，执行软删除
                                        todoManager.removeTodo(index);
                                    }

                                    // 延迟重新启用项目的MouseArea
                                    Qt.callLater(function () {
                                        if (itemMouseArea) {
                                            itemMouseArea.enabled = true;
                                        }
                                    });
                                }
                            }
                        }

                        // 左滑手势处理
                        MouseArea {
                            id: swipeArea
                            anchors.fill: parent
                            enabled: !multiSelectMode
                            pressAndHoldInterval: 500  // 长按时间为500毫秒
                            propagateComposedEvents: true  // 允许事件传递到子组件

                            property real startX: 0
                            property bool isDragging: false
                            property bool isLongPressed: false  // 跟踪长按状态

                            // 按下时记录初始位置
                            onPressed: function (mouse) {
                                // 如果点击在删除区域，不处理
                                if (delegateItem.swipeOffset < 0 && mouse.x > parent.width + delegateItem.swipeOffset) {
                                    mouse.accepted = false;
                                    return;
                                }
                                startX = mouse.x;
                                isDragging = false;
                                isLongPressed = false;  // 重置长按状态
                            }

                            // 拖动时处理
                            onPositionChanged: function (mouse) {
                                if (pressed) {
                                    // 如果点击在删除区域，不处理拖动
                                    if (delegateItem.swipeOffset < 0 && mouse.x > parent.width + delegateItem.swipeOffset) {
                                        mouse.accepted = false;
                                        return;
                                    }

                                    var deltaX = mouse.x - startX;
                                    if (Math.abs(deltaX) > 20) {
                                        // 增加拖拽阈值到20像素
                                        isDragging = true;
                                    }

                                    if (isDragging && deltaX < 0) {
                                        delegateItem.swipeOffset = Math.max(deltaX, -80);
                                        delegateItem.swipeActive = true;
                                    } else if (isDragging && deltaX > 0 && delegateItem.swipeOffset < 0) {
                                        // 允许向右滑动回弹
                                        delegateItem.swipeOffset = Math.min(0, delegateItem.swipeOffset + deltaX);
                                    }
                                }
                            }

                            // 释放时处理
                            onReleased: function (mouse) {
                                // 如果点击在删除区域，直接传递事件
                                if (delegateItem.swipeOffset < 0 && mouse.x > parent.width + delegateItem.swipeOffset) {
                                    mouse.accepted = false;
                                    isDragging = false;
                                    delegateItem.swipeActive = false;
                                    return;
                                }

                                if (isDragging) {
                                    // 如果滑动距离不够，回弹
                                    if (delegateItem.swipeOffset > -40) {
                                        swipeResetAnimation.start();
                                    }
                                } else if (!delegateItem.swipeActive && !isLongPressed) {
                                    // 如果不是滑动且未触发长按，执行点击逻辑
                                    if (multiSelectMode) {
                                        // 多选模式下切换选中状态
                                        var newSelectedItems = selectedItems.slice();
                                        var itemIndex = newSelectedItems.indexOf(index);
                                        if (itemIndex !== -1) {
                                            newSelectedItems.splice(itemIndex, 1);
                                        } else {
                                            newSelectedItems.push(index);
                                        }
                                        selectedItems = newSelectedItems;

                                        // 如果没有选中项，退出多选模式
                                        if (selectedItems.length === 0) {
                                            multiSelectMode = false;
                                        }
                                    } else {
                                        // 普通模式下显示详情
                                        selectedTodo = {
                                            title: model.title,
                                            description: model.description,
                                            category: model.category,
                                            important: model.important
                                        };
                                        todoListView.currentIndex = index;
                                    }
                                    mouse.accepted = false;  // 不是拖拽，让下层处理点击
                                }
                                isDragging = false;
                                delegateItem.swipeActive = false;
                            }

                            // 长按处理
                            onPressAndHold: function (mouse) {
                                if (!isDragging) {
                                    isLongPressed = true;  // 标记已触发长按
                                    // 长按进入多选模式
                                    if (!multiSelectMode) {
                                        multiSelectMode = true;
                                        selectedItems = [index];
                                    }
                                }
                            }
                        }

                        // 滑动回弹动画
                        NumberAnimation {
                            id: swipeResetAnimation
                            target: delegateItem
                            property: "swipeOffset"
                            to: 0
                            duration: 200
                            easing.type: Easing.OutQuad
                        }
                    }

                    // 多选模式下的操作栏
                    Rectangle {
                        visible: multiSelectMode
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 50
                        color: theme.backgroundColor || "white"
                        border.width: 1
                        border.color: theme.borderColor || "lightgray"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8

                            Label {
                                text: qsTr("已选择 %1 项").arg(selectedItems.length)
                                color: theme.textColor
                                Layout.fillWidth: true
                            }

                            Button {
                                text: qsTr("取消")
                                onClicked: {
                                    multiSelectMode = false;
                                    selectedItems = [];
                                }
                            }

                            Button {
                                text: qsTr("删除")
                                enabled: selectedItems.length > 0
                                onClicked: {
                                    // 检查是否在回收站中
                                    if (todoFilter.currentFilter === "recycle") {
                                        // 在回收站中，弹出确认弹窗进行硬删除
                                        confirmDeleteDialog.selectedIndices = selectedItems.slice();
                                        confirmDeleteDialog.open();
                                    } else {
                                        // 不在回收站中，执行软删除
                                        var sortedIndices = selectedItems.slice().sort(function (a, b) {
                                            return b - a;
                                        });
                                        for (var i = 0; i < sortedIndices.length; i++) {
                                            todoManager.removeTodo(sortedIndices[i]);
                                        }
                                        multiSelectMode = false;
                                        selectedItems = [];
                                    }
                                }
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
                            onClicked: root.showMinimized()
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
                            text: "\ue955"                      ///< 更多图标
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
    }

    // 种类筛选菜单（从筛选按钮点击弹出）
    Menu {
        id: categoryFilterMenu
        width: 200
        height: implicitHeight
        z: 10000  // 确保菜单显示在最上层

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

    // 确认删除弹窗
    BaseDialog {
        id: confirmDeleteDialog
        property var selectedIndices: []

        dialogTitle: qsTr("确认删除")
        dialogWidth: 350
        dialogHeight: 150

        Text {
            text: qsTr("确定要永久删除选中的 %1 个待办事项吗？\n此操作无法撤销。").arg(confirmDeleteDialog.selectedIndices.length)
            color: theme.textColor
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        // 执行硬删除
        onAccepted: {
            var sortedIndices = confirmDeleteDialog.selectedIndices.slice().sort(function (a, b) {
                return b - a;
            });
            for (var i = 0; i < sortedIndices.length; i++) {
                todoManager.permanentlyDeleteTodo(sortedIndices[i]);
            }
            multiSelectMode = false;
            selectedItems = [];
        }

        // 取消删除，不做任何操作
        onRejected: {}
    }
}
