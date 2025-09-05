import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

// 待办事项列表
ListView {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true

    // 使用 C++ 的 todoManager
    model: todoManager

    // 外部导入的组件
    property var selectedTodo

    property bool multiSelectMode: false  // 多选模式
    property var selectedItems: []        // 选中的项目索引列表

    // 下拉刷新相关属性与逻辑
    property int pullThreshold: 20 // 下拉刷新触发阈值
    property real pullDistance: 0  // 当前下拉距离

    // 智能时间格式化函数
    function formatDateTime(dateTime) {
        if (!dateTime)
            return "";

        var now = new Date();
        var date = new Date(dateTime);

        // 计算时间差（毫秒）
        var timeDiff = now.getTime() - date.getTime();
        var minutesDiff = Math.floor(timeDiff / (1000 * 60));
        var hoursDiff = Math.floor(timeDiff / (1000 * 60 * 60));
        var daysDiff = Math.floor(timeDiff / (1000 * 60 * 60 * 24));

        // 今天
        if (daysDiff === 0) {
            // 小于1分钟
            if (minutesDiff < 1) {
                return qsTr("刚刚");
            } else
            // 小于1小时
            if (hoursDiff < 1) {
                return minutesDiff + qsTr("分钟前");
            } else
            // 显示具体时间
            {
                var hours = date.getHours();
                var minutes = date.getMinutes();
                return String(hours).padStart(2, '0') + ":" + String(minutes).padStart(2, '0');
            }
        } else
        // 昨天
        if (daysDiff === 1) {
            return qsTr("昨天");
        } else
        // 前天
        if (daysDiff === 2) {
            return qsTr("前天");
        } else
        // 今年内
        if (date.getFullYear() === now.getFullYear()) {
            var month = date.getMonth() + 1;
            var day = date.getDate();
            return String(month).padStart(2, '0') + "/" + String(day).padStart(2, '0');
        } else
        // 跨年
        {
            var year = date.getFullYear();
            var month = date.getMonth() + 1;
            var day = date.getDate();
            return year + "/" + String(month).padStart(2, '0') + "/" + String(day).padStart(2, '0');
        }
    }

    onContentYChanged: {
        pullDistance = contentY < 0 ? -contentY : 0;
    }

    onMovementEnded: {
        // 如果下拉距离超过阈值且在顶部，触发刷新
        if (contentY < -pullThreshold && atYBeginning && !globalState.refreshing) {
            console.info("下拉刷新触发");
            globalState.refreshing = true;
            todoManager.syncWithServer();
        } else {}
    }

    header: Item {
        width: root.width
        height: globalState.refreshing ? 50 : Math.min(50, root.pullDistance)
        visible: height > 0 || globalState.refreshing
        RowLayout {
            anchors.centerIn: parent
            spacing: 6
            // 刷新
            IconButton {
                id: refreshIndicatorIcon
                text: "\ue8e2"              // 刷新图标
                visible: globalState.refreshing || root.pullDistance > 0
                width: 20
                height: 20
                Layout.alignment: Qt.AlignVCenter

                // 根据下拉比例旋转
                rotation: {
                    if (globalState.refreshing) {
                        console.info("刷新时保持旋转动画");
                        // 刷新时保持旋转动画
                        return 0; // 由下面的RotationAnimation控制
                    } else {
                        // 根据下拉比例计算旋转角度 (0-360度)
                        var ratio = Math.min(root.pullDistance / (root.pullThreshold * 5), 1.0);
                        return ratio * 360;
                    }
                }

                // 刷新时的旋转动画
                RotationAnimation {
                    target: refreshIndicatorIcon
                    from: 0
                    to: 360
                    duration: 1000
                    loops: Animation.Infinite
                    running: globalState.refreshing
                }
            }
            Label {
                text: globalState.refreshing ? qsTr("正在同步...") : (root.pullDistance >= root.pullThreshold ? qsTr("释放刷新") : qsTr("下拉刷新"))
                color: ThemeManager.textColor
                font.pixelSize: 12
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    // 下拉距离重置动画
    NumberAnimation {
        id: pullDistanceAnimation
        target: root
        property: "pullDistance"
        to: 0
        duration: 300
        easing.type: Easing.OutCubic
    }

    Connections {
        target: todoManager
        function onSyncStarted() {
            console.info("开始同步");
            if (!globalState.refreshing && root.atYBeginning) {
                globalState.refreshing = true;
            }
        }
        function onSyncCompleted(result, message) {
            console.info("同步完成", result, message);
            globalState.refreshing = false;
            // 使用动画平滑重置下拉距离
            pullDistanceAnimation.start();
            // 强制刷新ListView以确保显示最新数据
            root.model = null;
            root.model = todoManager;
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
            color: ThemeManager.backgroundColor
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
                            index: index,
                            title: model.title,
                            description: model.description,
                            category: model.category,
                            priority: model.priority,
                            completed: model.completed,
                            createdAt: model.createdAt,
                            lastModifiedAt: model.lastModifiedAt,
                            completedAt: model.completedAt,
                            deletedAt: model.deletedAt,
                            deadline: model.deadline,
                            recurrenceInterval: model.recurrenceInterval,
                            recurrenceCount: model.recurrenceCount,
                            recurrenceStartDate: model.recurrenceStartDate,
                            important: model.important
                        };
                        root.currentIndex = index;
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

            // 内容区域
            // 圆角边框
            Rectangle {
                anchors.fill: parent
                radius: 10
                border.width: 1
                border.color: ThemeManager.borderColor
                anchors.leftMargin: 2
                anchors.rightMargin: 2
                anchors.topMargin: 2
                anchors.bottomMargin: 0

                color: delegateItem.isSelected ? ThemeManager.selectedColor || "lightblue" : (index % 2 === 0 ? ThemeManager.secondaryBackgroundColor : ThemeManager.backgroundColor)

                RowLayout {
                    anchors.fill: parent
                    spacing: 8
                    ColumnLayout {
                        spacing: 1
                        Item {
                            Layout.fillHeight: true
                        }

                        // “是否重要等”和“标题”
                        RowLayout {
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8
                            spacing: 8

                            // 待办状态指示器
                            Rectangle {
                                width: 10
                                height: 10
                                radius: 4
                                color: (model.isCompleted || false) ? ThemeManager.completedColor : (model.important || false) ? ThemeManager.highImportantColor : ThemeManager.lowImportantColor

                                // TODO ：如果是备忘录模式记得开启它
                                // MouseArea {
                                //     anchors.fill: parent
                                //     onClicked: function (mouse) {
                                //         if (!multiSelectMode) {
                                //             todoManager.markAsDone(index);
                                //         }
                                //         mouse.accepted = true;
                                //     }
                                // }
                            }

                            // 待办标题
                            Label {
                                text: model.title
                                font.strikeout: model.isCompleted || false
                                color: (model.isCompleted || false) ? ThemeManager.secondaryTextColor : ThemeManager.textColor
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                maximumLineCount: 1
                            }
                        }
                        // TODO:如果是记录（笔记或备忘录）
                        // “时间”和“部分内容”
                        RowLayout {
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8
                            spacing: 4
                            // 时间
                            Label {
                                text: formatDateTime(model.lastModifiedAt)
                                font.pixelSize: 12
                                color: ThemeManager.secondaryTextColor
                            }
                            Label {
                                text: "|"
                                font.pixelSize: 12
                                color: ThemeManager.secondaryTextColor
                                visible: model.description !== ""
                            }
                            // 部分内容
                            Label {
                                text: model.description
                                font.pixelSize: 12
                                color: ThemeManager.secondaryTextColor
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                maximumLineCount: 1
                            }
                        }

                        Item {
                            Layout.fillHeight: true
                        }
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
        }

        // TODO： 现在左右滑动没让主体也跟着滑动
        // 右划完成背景
        Rectangle {
            id: completeBackground
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: Math.abs(delegateItem.swipeOffset)
            color: (model.isCompleted || false) ? ThemeManager.warningColor : ThemeManager.successColor
            visible: delegateItem.swipeOffset > 0

            Text {
                anchors.centerIn: parent
                text: (model.isCompleted || false) ? qsTr("未完成") : qsTr("已完成")
                color: ThemeManager.backgroundColor
                font.pixelSize: 14
            }

            MouseArea {
                id: completeMouseArea
                anchors.fill: parent
                z: 10  // 确保在最上层
                hoverEnabled: true
                onClicked: {
                    itemMouseArea.enabled = false; // 先禁用以防多次点
                    root.currentIndex = index; // 设置当前项索引
                    todoManager.markAsDoneOrTodo(index);     // 切换完成状态
                    // model.isCompleted = !model.isCompleted;

                    // 延迟重新启用项目的MouseArea
                    Qt.callLater(function () {
                        if (itemMouseArea) {
                            itemMouseArea.enabled = true;
                        }
                    });
                }
            }
        }

        // 左划删除背景
        Rectangle {
            id: deleteBackground
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: Math.abs(delegateItem.swipeOffset)
            color: ThemeManager.errorColor
            visible: delegateItem.swipeOffset < 0

            Text {
                anchors.centerIn: parent
                text: qsTr("删除")
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
                    root.currentIndex = index; // 设置当前项索引

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

        // 左划手势处理
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
                // 如果点击在完成区域，不处理
                if (delegateItem.swipeOffset > 0 && mouse.x < delegateItem.swipeOffset) {
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
                    // 如果点击在完成区域，不处理拖动
                    if (delegateItem.swipeOffset > 0 && mouse.x < delegateItem.swipeOffset) {
                        mouse.accepted = false;
                        return;
                    }

                    var deltaX = mouse.x - startX;
                    if (Math.abs(deltaX) > 20) {
                        // 增加拖拽阈值到20像素
                        isDragging = true;
                    }

                    if (isDragging && deltaX < 0) {
                        // 左划删除
                        delegateItem.swipeOffset = Math.max(deltaX, -80);
                        delegateItem.swipeActive = true;
                    } else if (isDragging && deltaX > 0) {
                        if (delegateItem.swipeOffset < 0) {
                            // 从左划状态向右滑动回弹
                            delegateItem.swipeOffset = Math.min(0, delegateItem.swipeOffset + deltaX);
                        } else {
                            // 右划切换完成状态
                            delegateItem.swipeOffset = Math.min(deltaX, 80);
                            delegateItem.swipeActive = true;
                        }
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
                // 如果点击在完成区域，直接传递事件
                if (delegateItem.swipeOffset > 0 && mouse.x < delegateItem.swipeOffset) {
                    mouse.accepted = false;
                    isDragging = false;
                    delegateItem.swipeActive = false;
                    return;
                }

                if (isDragging) {
                    // 如果左划距离不够，回弹
                    if (delegateItem.swipeOffset > -40 && delegateItem.swipeOffset < 0) {
                        swipeResetAnimation.start();
                    } else
                    // 如果右划距离不够，回弹
                    if (delegateItem.swipeOffset < 40 && delegateItem.swipeOffset > 0) {
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
                        // 普通模式（点击一次）下显示详情
                        selectedTodo = {
                            title: model.title,
                            description: model.description,
                            category: model.category,
                            priority: model.priority,
                            completed: model.completed,
                            createdAt: model.createdAt,
                            lastModifiedAt: model.lastModifiedAt,
                            completedAt: model.completedAt,
                            deletedAt: model.deletedAt,
                            deadline: model.deadline,
                            recurrenceInterval: model.recurrenceInterval,
                            recurrenceCount: model.recurrenceCount,
                            recurrenceStartDate: model.recurrenceStartDate
                        };
                        root.currentIndex = index;
                    }
                    mouse.accepted = true;  // 已处理点击事件，阻止事件传递
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
        height: 60
        color: ThemeManager.backgroundColor || "white"
        border.width: 1
        border.color: ThemeManager.borderColor || "lightgray"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 2

            Label {
                text: qsTr("已选择 %1 项").arg(selectedItems.length)
                color: ThemeManager.textColor
                Layout.leftMargin: 4
            }

            CustomButton {
                text: qsTr("取消")
                is2ndColor: true
                implicitWidth: 50
                implicitHeight: 36
                fontSize: 12
                onClicked: {
                    multiSelectMode = false;
                    selectedItems = [];
                }
            }

            CustomButton {
                text: qsTr("删除")
                enabled: selectedItems.length > 0
                backgroundColor: ThemeManager.errorColor
                hoverColor: Qt.darker(ThemeManager.errorColor, 1.1)
                pressedColor: Qt.darker(ThemeManager.errorColor, 1.2)
                implicitWidth: 50
                implicitHeight: 36
                fontSize: 12
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
}
