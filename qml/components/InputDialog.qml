/**
 * @file InputDialog.qml
 * @brief 单个输入对话框组件
 *
 * 基于 BaseDialog 的输入对话框，用于获取用户输入的文本内容。
 * 支持输入验证和自定义提示信息。
 *
 * @author Sakurakugu
 * @date 2025-09-02 15:32:07(UTC+8) 周二
 * @change 2025-09-03 22:49:59(UTC+8) 周三
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
    property string errorMessage: ""                  ///< 错误信息
    property bool isValid: true                       ///< 输入是否有效
    property int maxLength: 50                        ///< 最大输入长度

    // 信号
    signal inputAccepted(string text)                 ///< 输入确认信号

    // 对话框设置
    dialogWidth: 280
    maxDialogHeight: root.isValid ? 200 : 230
    contentSpacing: 10  // 减少内容区域间距

    // 错误消息显示区域
    Label {
        text: root.errorMessage
        color: ThemeManager.errorColor
        font.pixelSize: 12
        Layout.alignment: Qt.AlignHCenter         // 组件居中对齐
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter    // 文本居中对齐
        wrapMode: Text.WordWrap                   // 自动换行
        visible: !root.isValid && root.errorMessage !== ""
    }

    // 输入标签
    Label {
        text: root.inputLabel
        color: ThemeManager.textColor
        font.pixelSize: 14
        visible: root.inputLabel !== ""
        Layout.fillWidth: true
    }

    // 输入框
    CustomTextInput {
        id: inputField
        text: root.inputText
        placeholderText: root.placeholderText
        maximumLength: root.maxLength
        Layout.fillWidth: true

        onTextChanged: {
            if (!root.isValid && root.errorMessage !== "") {
                root.errorMessage = "";
                root.isValid = true;
            }
        }
    }

    // 按钮处理
    onConfirmed: {
        if (!validateWithCustom()) {
            return;
        }
        if (isValid && inputField.text.trim() !== "") {
            inputAccepted(inputField.text.trim());
        }
    }

    onCancelled: {
        root.clearInput();
        root.close();
    }

    // 打开时聚焦到输入框
    onOpened: {
        inputField.forceActiveFocus();
        inputField.selectAll();
    }

    // 验证输入内容
    function validateInput() {
        var text = inputField.text.trim();

        if (text === "") {
            isValid = false;
            errorMessage = qsTr("请输入内容");
            return false;
        }

        if (text.length > maxLength) {
            isValid = false;
            errorMessage = qsTr("输入内容过长");
            return false;
        }

        isValid = true;
        errorMessage = "";
        return true;
    }

    /**
     * @brief 设置输入内容
     * @param text 输入文本
     */
    function setInputText(text) {
        inputField.text = text;
        errorMessage = "";
    }

    /**
     * @brief 清空输入内容
     */
    function clearInput() {
        inputField.text = "";
        errorMessage = "";
    }

    /**
     * @brief 设置自定义验证函数
     * @param validationFunc 验证函数，返回 {valid: boolean, message: string}
     */
    property var customValidation: null

    function validateWithCustom() {
        if (customValidation && typeof customValidation === "function") {
            var result = customValidation(inputField.text.trim());
            isValid = result.valid;
            errorMessage = result.message || "";
            return result.valid;
        }
        return validateInput();
    }
}
