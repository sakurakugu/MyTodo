// 种类选择菜单（用于修改待办事项分类）

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    Menu {
        id: root
        width: 150
        z: 10000  // 确保菜单显示在最上层

        property bool categoryMultiSelectMode: false  // 分类多选模式
        property var categorySelectedItems: []        // 选中的分类列表
        property bool categoryFilterMultiSelectMode: false  // 分类筛选多选模式
        property var categoryFilterSelectedItems: []        // 选中的分类筛选列表
        property bool isFilterMode: false // 是否启用筛选模式

        ThemeManager {
            id: theme
        }

        // 确保只能打开一个菜单实例
        onAboutToShow: {
            // 如果菜单已经打开，先关闭
            if (opened) {
                close();
            }
            // 获取焦点以支持键盘操作
            forceActiveFocus();
        }

        // 当菜单失去焦点时自动关闭（除非在多选模式下）
        onActiveFocusChanged: {
            if (!activeFocus && !categoryMultiSelectMode && opened) {
                Qt.callLater(function () {
                    if (!activeFocus && opened) {
                        close();
                    }
                });
            }
        }

        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 4
        }

        // 分类筛选
        MenuItem {
            text: root.isFilterMode ? qsTr("分类筛选") : qsTr("选择分类")
            enabled: false
            height: visible ? implicitHeight : 0  // 解决不显示时的空白高度问题
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.bold: true
                font.pixelSize: 14
            }
        }

        MenuItem {
            text: qsTr("全部")
            visible: root.isFilterMode
            height: visible ? implicitHeight : 0  // 解决不显示时的空白高度问题
            onTriggered: {
                todoFilter.currentCategory = "";
            }
            contentItem: Rectangle {
                anchors.fill: parent
                implicitHeight: 30
                color: {
                    if (mouseArea_All.containsMouse)
                        return theme.hoverColor;
                    return "transparent";
                }

                RowLayout {
                    anchors.fill: parent
                    Text {
                        text: qsTr("全部")
                        color: theme.textColor
                        font.pixelSize: 12
                        Layout.fillWidth: true
                        Layout.leftMargin: 18
                    }
                }

                MouseArea {
                    id: mouseArea_All
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: parent.parent.triggered()
                }
            }
        }

        // 动态分类菜单项将通过Repeater添加
        Repeater {
            model: categoryManager.categories
            MenuItem {
                text: modelData
                onTriggered: {
                    if (!root.categoryMultiSelectMode) {
                        if (root.isFilterMode) {
                            todoFilter.currentCategory = modelData;
                        } else {
                            todoManager.updateTodo(selectedTodo.index, "category", modelData);
                            root.close();  // 选择后关闭菜单
                        }
                    }
                }

                property bool isSelected: root.categorySelectedItems.indexOf(modelData) !== -1 // 分类筛选多选模式下是否选中

                contentItem: Rectangle {
                    anchors.fill: parent
                    implicitHeight: 30
                    color: {
                        if (parent.isSelected)
                            return theme.selectedColor;
                        if (mouseArea.containsMouse) // 鼠标悬停
                            return theme.hoverColor;
                        return "transparent";
                    }

                    RowLayout {
                        anchors.fill: parent
                        Text {
                            id: categoryText
                            text: modelData
                            color: theme.textColor
                            font.pixelSize: 12
                            Layout.fillWidth: true
                            Layout.leftMargin: 18
                        }

                        CheckBox {
                            visible: root.categoryMultiSelectMode && modelData !== "未分类"
                            enabled: modelData !== "未分类"
                            checked: parent.parent.parent.isSelected  // 修复绑定路径
                            Layout.preferredWidth: visible ? implicitWidth : 0
                            onClicked: {
                                root.toggleCategorySelection(modelData);
                            }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        hoverEnabled: true  // 启用悬停效果

                        onClicked: {
                            if (root.categoryMultiSelectMode) {
                                // 未分类不能被选中
                                if (modelData !== "未分类") {
                                    root.toggleCategorySelection(modelData);
                                }
                            } else {
                                if (root.isFilterMode) {
                                    todoFilter.currentCategory = modelData;
                                } else {
                                    todoManager.updateTodo(selectedTodo.index, "category", modelData);
                                }
                                root.close();
                            }
                        }

                        onPressAndHold: {
                            // 长按进入多选模式，但未分类不能被选中
                            if (!root.categoryMultiSelectMode && modelData !== "未分类") {
                                root.categoryMultiSelectMode = true;
                                root.categorySelectedItems = [modelData];
                            }
                        }
                    }
                }
            }
        }

        MenuSeparator {
            contentItem: Rectangle {
                implicitHeight: 1
                color: theme.borderColor
            }
        }

        MenuItem {
            text: qsTr("新增种类")
            onTriggered: {
                addCategoryDialog.open();
                root.close();  // 打开对话框后关闭菜单
            }
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
                font.bold: true
            }
        }

        // 多选模式下的操作栏
        MenuSeparator {
            visible: root.categoryMultiSelectMode
            height: visible ? implicitHeight : 0  // 解决不显示时的空白高度问题
            contentItem: Rectangle {
                implicitHeight: 1
                color: theme.borderColor
            }
        }

        MenuItem {
            visible: root.categoryMultiSelectMode && root.categorySelectedItems.length === 1
            height: visible ? implicitHeight : 0  // 解决不显示时的空白高度问题
            text: qsTr("修改名称")
            enabled: root.categorySelectedItems.length === 1
            onTriggered: {
                // 修改选中分类的名称
                if (root.categorySelectedItems.length === 1) {
                    editCategoryDialog.currentCategoryName = root.categorySelectedItems[0];
                    editCategoryDialog.open();
                }
            }
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
                font.bold: true
            }
        }

        MenuItem {
            visible: root.categoryMultiSelectMode
            height: visible ? implicitHeight : 0  // 解决不显示时的空白高度问题
            text: qsTr("删除选中分类 (") + root.categorySelectedItems.length + ")"
            enabled: root.categorySelectedItems.length > 0
            onTriggered: {
                // 删除选中的分类
                for (var i = 0; i < root.categorySelectedItems.length; i++) {
                    categoryManager.deleteCategory(root.categorySelectedItems[i]);
                }
                root.resetMultiSelectMode();
                root.close();
            }
            contentItem: Text {
                text: parent.text
                color: parent.enabled ? "#ff4444" : theme.secondaryTextColor
                font.pixelSize: 12
                font.bold: true
            }
        }

        MenuItem {
            visible: root.categoryMultiSelectMode
            height: visible ? implicitHeight : 0  // 解决不显示时的空白高度问题
            text: qsTr("取消")
            onTriggered: {
                root.resetMultiSelectMode();
            }
            contentItem: Text {
                text: parent.text
                color: theme.textColor
                font.pixelSize: 12
            }
        }

        // 优化：添加重置多选模式的函数
        function resetMultiSelectMode() {
            categoryMultiSelectMode = false;
            categorySelectedItems = [];
        }

        // 优化：添加切换分类选择的函数
        function toggleCategorySelection(category) {
            var newSelectedItems = categorySelectedItems.slice();
            var itemIndex = newSelectedItems.indexOf(category);
            if (itemIndex !== -1) {
                newSelectedItems.splice(itemIndex, 1);
            } else {
                newSelectedItems.push(category);
            }
            categorySelectedItems = newSelectedItems;

            // 如果没有选中项，退出多选模式
            if (categorySelectedItems.length === 0) {
                categoryMultiSelectMode = false;
            }
        }

        // 优化：添加键盘支持 - 使用Shortcut替代Keys
        Shortcut {
            sequence: "Escape"
            enabled: root.opened
            onActivated: {
                if (root.categoryMultiSelectMode) {
                    root.resetMultiSelectMode();
                } else {
                    root.close();
                }
            }
        }

        // 优化：添加性能优化 - 延迟加载分类列表
        property bool isMenuVisible: visible && opened

        // 计算菜单高度，避免绑定循环
        property int calculatedHeight: {
            var baseHeight = 0;
            // 分类筛选标题
            if (isFilterMode)
                baseHeight += 35;
            // "全部"选项
            if (isFilterMode)
                baseHeight += 35;
            // 动态分类项
            baseHeight += categoryManager.categories.length * 35;
            // 分隔符
            baseHeight += 5;
            // "新增种类"按钮
            baseHeight += 35;
            // 多选模式下的额外项
            if (root.categoryMultiSelectMode) {
                baseHeight += 5; // 分隔符
                if (root.categorySelectedItems.length === 1) {
                    baseHeight += 35; // 修改名称按钮
                }
                baseHeight += 35; // 删除按钮
                baseHeight += 35; // 取消按钮
            }
            return Math.min(baseHeight, 400); // 限制最大高度
        }

        height: calculatedHeight

        // 当菜单关闭时重置状态
        onClosed: {
            if (categoryMultiSelectMode) {
                resetMultiSelectMode();
            }
        }
    }
    
    // 新增种类对话框
    InputDialog {
        id: addCategoryDialog
        dialogTitle: qsTr("新增种类")
        inputLabel: qsTr("种类名称:")
        placeholderText: qsTr("请输入种类名称")
        maxLength: 20

        // 自定义验证函数
        customValidation: function (text) {
            if (text === "") {
                return {
                    valid: false,
                    message: qsTr("请输入种类名称")
                };
            }
            if (text.length > 20) {
                return {
                    valid: false,
                    message: qsTr("种类名称不能超过20个字符")
                };
            }
            if (categoryManager.categoryExists(text)) {
                return {
                    valid: false,
                    message: qsTr("该种类已存在")
                };
            }
            return {
                valid: true,
                message: ""
            };
        }

        onInputAccepted: function (text) {
            categoryManager.createCategory(text);
            addCategoryDialog.clearInput();
            root.close();
        }

        onInputRejected: {
            addCategoryDialog.clearInput();
            root.close();
        }
    }

    // 修改种类名称对话框
    InputDialog {
        id: editCategoryDialog
        dialogTitle: qsTr("修改种类名称")
        inputLabel: qsTr("种类名称:")
        placeholderText: qsTr("请输入新的种类名称")
        maxLength: 20

        property string currentCategoryName: ""

        onAboutToShow: {
            // 打开对话框时设置当前分类名称为默认值
            setInputText(currentCategoryName);
        }

        // 自定义验证函数
        customValidation: function (text) {
            if (text === "") {
                return {
                    valid: false,
                    message: qsTr("请输入种类名称")
                };
            }
            if (text.length > 20) {
                return {
                    valid: false,
                    message: qsTr("种类名称不能超过20个字符")
                };
            }
            if (text === currentCategoryName) {
                return {
                    valid: false,
                    message: qsTr("新名称不能与原名称相同")
                };
            }
            if (categoryManager.categoryExists(text)) {
                return {
                    valid: false,
                    message: qsTr("该种类已存在")
                };
            }
            return {
                valid: true,
                message: ""
            };
        }

        onInputAccepted: function (text) {
            categoryManager.updateCategory(currentCategoryName, text);
            editCategoryDialog.clearInput();
            root.resetMultiSelectMode();
            root.close();
        }

        onInputRejected: {
            editCategoryDialog.clearInput();
            root.close();
        }
    }

    // 种类选择菜单
    function popup(x, y, isFilterMode) {
        root.isFilterMode = isFilterMode;
        root.popup(x, y);
    }

    // 关闭种类选择菜单
    function close() {
        root.close();
    }
}
