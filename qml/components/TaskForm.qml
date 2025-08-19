/**
 * @file TaskForm.qml
 * @brief 任务表单组件
 * 
 * 提供创建和编辑任务的表单界面，包括：
 * - 任务标题和描述输入
 * - 分类、重要程度选择
 * - 紧凑模式和完整模式支持
 * - 主题色彩适配
 * - 表单验证和提交
 * 
 * @author MyTodo Team
 * @date 2024
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @brief 任务表单组件
 * 
 * 可重用的任务表单组件，支持创建和编辑任务。
 * 提供灵活的显示模式和完整的表单验证功能。
 */
ColumnLayout {
    id: taskForm
    
    // 表单数据属性（通过别名暴露内部控件的属性）
    property alias titleText: titleField.text              ///< 任务标题文本
    property alias descriptionText: descriptionField.text  ///< 任务描述文本
    property alias categoryIndex: categoryCombo.currentIndex    ///< 分类选择索引

    property alias importantIndex: importantCombo.currentIndex ///< 重要程度索引
    property alias categoryText: categoryCombo.currentText      ///< 分类文本

    property alias importantText: importantCombo.currentText  ///< 重要程度文本
    
    // 外观和行为属性
    property color textColor: "black"      ///< 文本颜色
    property bool showDescription: true    ///< 是否显示描述字段
    property bool showPriority: true       ///< 是否显示优先级字段
    property bool isCompactMode: false     ///< 是否为紧凑模式
    property var theme: null               ///< 主题对象
    
    // 表单事件信号
    signal formAccepted()                  ///< 表单提交信号
    
    spacing: 10                            ///< 控件间距
    
    /**
     * @brief 任务标题输入区域
     * 
     * 包含标题标签和输入框，支持回车提交。
     */
    Label {
        text: qsTr("标题")                   ///< 标题标签
        color: textColor                     ///< 标签颜色
    }
    TextField {
        id: titleField
        Layout.fillWidth: true               ///< 填充可用宽度
        color: textColor                     ///< 文本颜色
        placeholderText: qsTr("输入任务标题...") ///< 占位符文本
        
        /// 回车键提交表单
        onAccepted: {
            if (text.trim() !== "") {
                taskForm.formAccepted()
            }
        }
    }
    
    /**
     * @brief 任务描述输入区域
     * 
     * 可选的描述字段，在紧凑模式下隐藏。
     */
    Label {
        text: qsTr("描述")                   ///< 描述标签
        color: textColor                     ///< 标签颜色
        visible: showDescription && !isCompactMode ///< 根据设置和模式控制显示
    }
    TextArea {
        id: descriptionField
        Layout.fillWidth: true
        Layout.preferredHeight: 80
        wrapMode: TextEdit.Wrap
        color: textColor
        placeholderText: qsTr("输入任务描述...")
        visible: showDescription && !isCompactMode
    }
    
    // 分类字段
    Label {
        text: qsTr("分类")
        color: textColor
    }
    ComboBox {
        id: categoryCombo
        model: ["工作", "学习", "生活", "其他"]
        Layout.fillWidth: true
    }
    
    // 重要程度字段（可选）
    ColumnLayout {
        Layout.fillWidth: true
        visible: showPriority && !isCompactMode
        
        Label {
            text: qsTr("重要程度")
            color: textColor
        }
        ComboBox {
            id: importantCombo
            model: ["普通", "重要"]
            Layout.fillWidth: true
            currentIndex: 0 // 默认选择"普通"
        }
    }
    
    // 截止日期字段
    ColumnLayout {
        Layout.fillWidth: true
        visible: !isCompactMode
        
        Label {
            text: qsTr("截止日期")
            color: textColor
        }
        TextField {
            id: deadlineField
            Layout.fillWidth: true
            placeholderText: qsTr("YYYY-MM-DD HH:MM")
            color: textColor
        }
    }
    
    // 公开方法
    function clearForm() {
        titleField.text = ""
        descriptionField.text = ""
        categoryCombo.currentIndex = 0
        importantCombo.currentIndex = 0
        deadlineField.text = ""
    }
    
    function setFormData(todo) {
        titleField.text = todo.title || ""
        descriptionField.text = todo.description || ""
        categoryCombo.currentIndex = Math.max(0, categoryCombo.model.indexOf(todo.category))
        importantCombo.currentIndex = todo.important ? 1 : 0
        deadlineField.text = todo.deadline || ""
    }
    
    function focusTitle() {
        titleField.forceActiveFocus()
    }
    
    function getTodoData() {
        return {
            title: titleField.text.trim(),
            description: descriptionField.text.trim(),
            category: categoryCombo.currentText,
            important: importantCombo.currentIndex === 1, // true if "重要", false if "普通"
            deadline: deadlineField.text.trim()
        }
    }
    
    function isValid() {
        return titleField.text.trim() !== ""
    }
}