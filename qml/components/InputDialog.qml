/**
 * @file InputDialog.qml
 * @brief 输入对话框组件
 *
 * 基于 BaseDialog 的输入对话框，用于获取用户输入的文本内容。
 * 支持输入验证和自定义提示信息。
 *
 * @author Sakurakugu
 * @date 2025-01-XX
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    id: root

    // 公共属性
    property string inputLabel: ""                    ///< 输入框标签
    property string inputText: ""                     ///< 输入框文本
    property string placeholderText: ""               ///< 占位符文本
    property string validationMessage: ""             ///< 验证错误信息
    property bool isValid: true                       ///< 输入是否有效
    property int maxLength: 50                        ///< 最大输入长度
    property alias textField: inputField              ///< 输入框别名

    // 信号
    signal inputAccepted(string text)                 ///< 输入确认信号
    signal inputRejected()                            ///< 输入取消信号

    // 对话框设置
    dialogWidth: 350
    dialogHeight: 200
    standardButtonFlags: Dialog.Ok | Dialog.Cancel

    // 输入标签
    Label {
        text: root.inputLabel
        color: theme.textColor
        font.pixelSize: 14
        visible: root.inputLabel !== ""
        Layout.fillWidth: true
    }

    // 输入框
    TextField {
        id: inputField
        text: root.inputText
        placeholderText: root.placeholderText
        maximumLength: root.maxLength
        selectByMouse: true
        Layout.fillWidth: true
        Layout.preferredHeight: 40

        color: theme.textColor
        placeholderTextColor: theme.placeholderTextColor

        background: Rectangle {
            color: theme.cardBackgroundColor
            border.color: inputField.activeFocus ? theme.primaryColor : theme.borderColor
            border.width: inputField.activeFocus ? 2 : 1
            radius: 4
        }

        // 输入验证
        onTextChanged: {
            root.inputText = text
            validateInput()
        }

        // 回车键确认
        Keys.onReturnPressed: {
            if (root.isValid && text.trim() !== "") {
                root.accept()
            }
        }
    }

    // 验证错误信息
    Label {
        text: root.validationMessage
        color: theme.errorColor || "#ff4444"
        font.pixelSize: 12
        visible: !root.isValid && root.validationMessage !== ""
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }

    // 字符计数
    Label {
        text: inputField.text.length + "/" + root.maxLength
        color: theme.secondaryTextColor
        font.pixelSize: 10
        horizontalAlignment: Text.AlignRight
        Layout.fillWidth: true
    }

    // 按钮处理
    onAccepted: {
        if (isValid && inputField.text.trim() !== "") {
            inputAccepted(inputField.text.trim())
        }
    }

    onRejected: {
        inputRejected()
    }

    // 打开时聚焦到输入框
    onOpened: {
        inputField.forceActiveFocus()
        inputField.selectAll()
    }

    /**
     * @brief 验证输入内容
     */
    function validateInput() {
        var text = inputField.text.trim()
        
        if (text === "") {
            isValid = false
            validationMessage = qsTr("请输入内容")
            return false
        }
        
        if (text.length > maxLength) {
            isValid = false
            validationMessage = qsTr("输入内容过长")
            return false
        }
        
        isValid = true
        validationMessage = ""
        return true
    }

    /**
     * @brief 设置输入内容
     * @param text 输入文本
     */
    function setInputText(text) {
        inputField.text = text
    }

    /**
     * @brief 清空输入内容
     */
    function clearInput() {
        inputField.text = ""
    }

    /**
     * @brief 设置自定义验证函数
     * @param validationFunc 验证函数，返回 {valid: boolean, message: string}
     */
    property var customValidation: null

    function validateWithCustom() {
        if (customValidation && typeof customValidation === "function") {
            var result = customValidation(inputField.text.trim())
            isValid = result.valid
            validationMessage = result.message || ""
            return result.valid
        }
        return validateInput()
    }

    // 如果设置了自定义验证，使用自定义验证
    Component.onCompleted: {
        if (customValidation) {
            inputField.textChanged.connect(validateWithCustom)
        }
    }
}