/**
 * @file CustomTextEdit.qml
 * @brief 自定义多行文本编辑组件
 *
 * 基于TextEdit的自定义多行文本编辑组件，提供统一的样式和交互体验。
 * 支持主题切换、占位符文本、错误提示、滚动条、自动换行等功能。
 *
 * @author Sakurakugu
 * @date 2025-09-05 16:50:25(UTC+8) 周五
 * @change 2025-09-05 16:50:25(UTC+8) 周五
 */

import QtQuick
import QtQuick.Controls

Item {
    id: root

    // 公共属性
    property alias text: textEdit.text                         ///< 编辑文本
    property alias placeholderText: placeholderTextItem.text   ///< 占位符文本
    property alias font: textEdit.font                         ///< 字体设置
    property alias color: textEdit.color                       ///< 文本颜色
    property alias selectionColor: textEdit.selectionColor     ///< 选中文本的背景色
    property alias selectedTextColor: textEdit.selectedTextColor ///< 选中文本的颜色
    property alias readOnly: textEdit.readOnly                 ///< 是否只读
    property alias wrapMode: textEdit.wrapMode                 ///< 文本换行模式
    property alias cursorVisible: textEdit.cursorVisible       ///< 光标是否可见
    property alias cursorPosition: textEdit.cursorPosition     ///< 光标位置
    property alias horizontalAlignment: textEdit.horizontalAlignment ///< 水平对齐方式
    property alias verticalAlignment: textEdit.verticalAlignment ///< 垂直对齐方式
    property alias selectByMouse: textEdit.selectByMouse       ///< 是否可以通过鼠标选择文本
    property alias activeFocusOnPress: textEdit.activeFocusOnPress ///< 点击时是否获得焦点
    property alias textFormat: textEdit.textFormat             ///< 文本格式（纯文本/富文本）

    property string errorMessage: ""                            ///< 错误信息
    property bool isValid: true                                 ///< 输入是否有效
    property int radius: 4                                      ///< 圆角半径
    property int borderWidth: 1                                 ///< 边框宽度
    property color borderColor: ThemeManager.borderColor        ///< 边框颜色
    property color focusBorderColor: ThemeManager.primaryColor  ///< 焦点边框颜色
    property color errorBorderColor: ThemeManager.errorColor    ///< 错误边框颜色
    property color backgroundColor: ThemeManager.secondaryBackgroundColor ///< 背景颜色
    property int leftPadding: 10                                ///< 左内边距
    property int rightPadding: 10                               ///< 右内边距
    property int topPadding: 8                                  ///< 上内边距
    property int bottomPadding: 8                               ///< 下内边距

    // 滚动条相关属性
    property bool showScrollBar: true                           ///< 是否显示滚动条
    property color scrollBarColor: ThemeManager.borderColor     ///< 滚动条颜色
    property color scrollBarHandleColor: ThemeManager.primaryColor ///< 滚动条手柄颜色
    property int scrollBarWidth: 8                              ///< 滚动条宽度

    // 信号
    signal editingFinished                                      ///< 编辑完成信号（失去焦点）
    signal textEdited                                           ///< 文本编辑信号

    // 尺寸设置
    implicitWidth: 300
    implicitHeight: 120

    // 背景和边框
    Rectangle {
        id: background
        anchors.fill: parent
        color: root.backgroundColor
        radius: root.radius
        border.width: root.borderWidth
        border.color: {
            if (!root.isValid) {
                return root.errorBorderColor;
            }
            return textEdit.activeFocus ? root.focusBorderColor : root.borderColor;
        }

        // 颜色过渡动画
        Behavior on border.color {
            ColorAnimation {
                duration: 150
            }
        }
    }

    // 滚动视图
    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: root.borderWidth
        anchors.leftMargin: root.leftPadding
        anchors.rightMargin: root.rightPadding
        anchors.topMargin: root.topPadding
        anchors.bottomMargin: root.bottomPadding
        clip: true

        // 滚动条策略
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        // 自定义滚动条样式
        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBar
            width: root.scrollBarWidth
            policy: root.showScrollBar ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff

            contentItem: Rectangle {
                implicitWidth: root.scrollBarWidth
                radius: width / 2
                color: verticalScrollBar.pressed ? root.scrollBarHandleColor : verticalScrollBar.hovered ? Qt.lighter(root.scrollBarHandleColor, 1.2) : root.scrollBarColor
                opacity: verticalScrollBar.policy === ScrollBar.AlwaysOn || (verticalScrollBar.active && verticalScrollBar.size < 1.0) ? 1.0 : 0.0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 150
                    }
                }

                Behavior on color {
                    ColorAnimation {
                        duration: 150
                    }
                }
            }
        }

        // 文本编辑器
        TextEdit {
            id: textEdit
            width: scrollView.availableWidth
            color: ThemeManager.textColor
            selectionColor: ThemeManager.selectedColor
            selectedTextColor: ThemeManager.textColor
            font.pixelSize: 14
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            activeFocusOnPress: true
            textFormat: TextEdit.PlainText

            // 处理编辑完成
            onActiveFocusChanged: {
                if (!activeFocus) {
                    root.editingFinished();
                }
            }

            // 处理文本变化
            onTextChanged: {
                if (!root.isValid) {
                    root.isValid = true;
                    root.errorMessage = "";
                }
                root.textEdited();
            }
        }
    }

    // 占位符文本
    Text {
        id: placeholderTextItem
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.leftPadding
        anchors.rightMargin: root.rightPadding
        anchors.topMargin: root.topPadding
        font: textEdit.font
        color: ThemeManager.placeholderTextColor
        visible: !textEdit.text && !textEdit.activeFocus
        wrapMode: Text.Wrap
        verticalAlignment: Text.AlignTop
    }

    // 错误消息
    Text {
        id: errorText
        text: root.errorMessage
        color: ThemeManager.errorColor
        font.pixelSize: 12
        anchors.left: parent.left
        anchors.top: parent.bottom
        anchors.topMargin: 2
        visible: !root.isValid && root.errorMessage !== ""
        wrapMode: Text.Wrap
    }

    // 方法：设置焦点
    function forceActiveFocus() {
        textEdit.forceActiveFocus();
    }

    // 方法：选择所有文本
    function selectAll() {
        textEdit.selectAll();
    }

    // 方法：清除文本
    function clear() {
        textEdit.text = "";
    }

    // 方法：在当前光标位置插入文本
    function insert(position, text) {
        textEdit.insert(position, text);
    }

    // 方法：移除指定范围的文本
    function remove(start, end) {
        textEdit.remove(start, end);
    }

    // 方法：获取指定范围的文本
    function getText(start, end) {
        return textEdit.getText(start, end);
    }

    // 方法：滚动到指定位置
    function scrollToPosition(position) {
        textEdit.cursorPosition = position;
        // 确保光标可见
        var rect = textEdit.positionToRectangle(position);
        scrollView.contentItem.contentY = Math.max(0, Math.min(rect.y - scrollView.height / 2, textEdit.height - scrollView.height));
    }

    // 方法：滚动到底部
    function scrollToBottom() {
        scrollView.contentItem.contentY = Math.max(0, textEdit.height - scrollView.height);
    }

    // 方法：滚动到顶部
    function scrollToTop() {
        scrollView.contentItem.contentY = 0;
    }

    // 方法：验证输入
    function validate() {
        // 基本验证，子类可以重写此方法提供更复杂的验证
        if (textEdit.text.trim() === "") {
            root.isValid = false;
            root.errorMessage = qsTr("请输入内容");
            return false;
        }

        root.isValid = true;
        root.errorMessage = "";
        return true;
    }

    // 方法：获取行数
    function getLineCount() {
        return textEdit.lineCount;
    }

    // 方法：获取当前行号（从0开始）
    function getCurrentLine() {
        var position = textEdit.cursorPosition;
        var text = textEdit.text.substring(0, position);
        return text.split('\n').length - 1;
    }

    // 方法：获取当前列号（从0开始）
    function getCurrentColumn() {
        var position = textEdit.cursorPosition;
        var text = textEdit.text.substring(0, position);
        var lines = text.split('\n');
        return lines[lines.length - 1].length;
    }
}
