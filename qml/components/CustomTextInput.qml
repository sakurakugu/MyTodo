/**
 * @file CustomTextInput.qml
 * @brief 自定义文本输入框组件
 *
 * 基于TextInput的自定义文本输入框组件，提供统一的样式和交互体验。
 * 支持主题切换、占位符文本、错误提示、左右图标等功能。
 *
 * @author Sakurakugu
 */

import QtQuick

Item {
    id: root

    // 公共属性
    property alias text: textInput.text                         ///< 输入文本
    property alias placeholderText: placeholderTextItem.text    ///< 占位符文本
    property alias font: textInput.font                         ///< 字体设置
    property alias color: textInput.color                       ///< 文本颜色
    property alias selectionColor: textInput.selectionColor     ///< 选中文本的背景色
    property alias selectedTextColor: textInput.selectedTextColor ///< 选中文本的颜色
    property alias echoMode: textInput.echoMode                 ///< 文本显示模式（密码等）
    property alias readOnly: textInput.readOnly                 ///< 是否只读
    property alias inputMethodHints: textInput.inputMethodHints ///< 输入法提示
    property alias validator: textInput.validator               ///< 输入验证器
    property alias maximumLength: textInput.maximumLength       ///< 最大输入长度
    property alias cursorVisible: textInput.cursorVisible       ///< 光标是否可见
    property alias cursorPosition: textInput.cursorPosition     ///< 光标位置
    property alias horizontalAlignment: textInput.horizontalAlignment ///< 水平对齐方式
    property alias verticalAlignment: textInput.verticalAlignment ///< 垂直对齐方式
    property alias selectByMouse: textInput.selectByMouse       ///< 是否可以通过鼠标选择文本
    property alias activeFocusOnPress: textInput.activeFocusOnPress ///< 点击时是否获得焦点
    
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
    property int topPadding: 5                                  ///< 上内边距
    property int bottomPadding: 5                               ///< 下内边距
    
    // 图标相关属性
    property string leftIcon: ""                               ///< 左侧图标（iconfont编码）
    property string rightIcon: ""                                ///< 右侧图标（iconfont编码）
    property color iconColor: ThemeManager.textColor            ///< 图标颜色
    property int iconSize: 16                                   ///< 图标大小
    property string iconFont: "iconfont"                        ///< 图标字体
    property int iconMargin: 10                                 ///< 图标边距
    
    // 图标信号
    signal leftIconClicked()                                    ///< 左侧图标点击信号
    signal rightIconClicked()                                   ///< 右侧图标点击信号

    // 信号
    signal accepted()                                           ///< 输入确认信号（回车键）
    signal editingFinished()                                    ///< 编辑完成信号（失去焦点）
    signal textEdited()                                         ///< 文本编辑信号

    // 尺寸设置
    implicitWidth: 200
    implicitHeight: 40

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
            return textInput.activeFocus ? root.focusBorderColor : root.borderColor;
        }

        // 颜色过渡动画
        Behavior on border.color {
            ColorAnimation {
                duration: 150
            }
        }
    }

    // 文本输入
    TextInput {
        id: textInput
        anchors.fill: parent
        anchors.leftMargin: root.leftIcon ? (root.leftPadding + root.iconSize + root.iconMargin) : root.leftPadding
        anchors.rightMargin: root.rightIcon ? (root.rightPadding + root.iconSize + root.iconMargin) : root.rightPadding
        anchors.topMargin: root.topPadding
        anchors.bottomMargin: root.bottomPadding
        color: ThemeManager.textColor
        selectionColor: ThemeManager.selectedColor
        selectedTextColor: ThemeManager.textColor
        font.pixelSize: 14
        clip: true
        selectByMouse: true
        verticalAlignment: TextInput.AlignVCenter

        // 处理回车键
        Keys.onReturnPressed: {
            root.accepted();
        }

        // 处理编辑完成
        onEditingFinished: {
            root.editingFinished();
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

    // 占位符文本
    Text {
        id: placeholderTextItem
        anchors.fill: textInput
        verticalAlignment: textInput.verticalAlignment
        horizontalAlignment: textInput.horizontalAlignment
        font: textInput.font
        color: ThemeManager.placeholderTextColor
        visible: !textInput.text && !textInput.activeFocus
        elide: Text.ElideRight
    }
    
    // 左侧图标
    Text {
        id: leftIconItem
        visible: root.leftIcon !== ""
        text: root.leftIcon
        color: root.iconColor
        font.pixelSize: root.iconSize
        font.family: root.iconFont
        anchors.left: parent.left
        anchors.leftMargin: root.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        
        MouseArea {
            anchors.fill: parent
            anchors.margins: -5  // 扩大点击区域
            onClicked: root.leftIconClicked()
        }
    }
    
    // 右侧图标
    Text {
        id: rightIconItem
        visible: root.rightIcon !== ""
        text: root.rightIcon
        color: root.iconColor
        font.pixelSize: root.iconSize
        font.family: root.iconFont
        anchors.right: parent.right
        anchors.rightMargin: root.rightPadding
        anchors.verticalCenter: parent.verticalCenter
        
        MouseArea {
            anchors.fill: parent
            anchors.margins: -5  // 扩大点击区域
            onClicked: root.rightIconClicked()
        }
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
    }

    // 方法：设置焦点
    function forceActiveFocus() {
        textInput.forceActiveFocus();
    }

    // 方法：选择所有文本
    function selectAll() {
        textInput.selectAll();
    }

    // 方法：清除文本
    function clear() {
        textInput.text = "";
    }

    // 方法：验证输入
    function validate() {
        // 基本验证，子类可以重写此方法提供更复杂的验证
        if (textInput.text.trim() === "") {
            root.isValid = false;
            root.errorMessage = qsTr("请输入内容");
            return false;
        }
        
        root.isValid = true;
        root.errorMessage = "";
        return true;
    }
}