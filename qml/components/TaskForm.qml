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
 * @author Sakurakugu
 * @date 2025-08-17 07:17:29(UTC+8) 周日
 * @version 2025-08-22 18:51:19(UTC+8) 周五
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
    signal formAccepted                  ///< 表单提交信号

    spacing: 10                            ///< 控件间距

    /**
     * @brief 任务标题输入区域
     *
     * 包含标题标签和输入框，支持回车提交。
     */
    Label {
        text: qsTr("标题")                   ///< 标题标签
        color: taskForm.textColor                     ///< 标签颜色
    }
    TextField {
        id: titleField
        Layout.fillWidth: true               ///< 填充可用宽度
        color: taskForm.textColor                     ///< 文本颜色
        placeholderText: qsTr("输入任务标题...") ///< 占位符文本

        /// 回车键提交表单
        onAccepted: {
            if (text.trim() !== "") {
                taskForm.formAccepted();
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
        color: taskForm.textColor                     ///< 标签颜色
        visible: showDescription && !isCompactMode ///< 根据设置和模式控制显示
    }
    TextArea {
        id: descriptionField
        Layout.fillWidth: true
        Layout.preferredHeight: 80
        wrapMode: TextEdit.Wrap
        color: taskForm.textColor
        placeholderText: qsTr("输入任务描述...")
        visible: showDescription && !isCompactMode
    }

    // 分类字段
    Label {
        text: qsTr("分类")
        color: taskForm.textColor
    }
    ComboBox {
        id: categoryCombo
        model: {
            // 从todoModel.categories中过滤掉"全部"选项
            var filteredCategories = [];
            for (var i = 0; i < todoModel.categories.length; i++) {
                if (todoModel.categories[i] !== "全部") {
                    filteredCategories.push(todoModel.categories[i]);
                }
            }
            return filteredCategories;
        }
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

    // 循环间隔字段
    ColumnLayout {
        Layout.fillWidth: true
        visible: !isCompactMode

        Label {
            text: qsTr("循环间隔（天）")
            color: textColor
        }
        TextField {
            id: recurrenceIntervalField
            Layout.fillWidth: true
            placeholderText: qsTr("0表示不循环")
            color: textColor
            validator: IntValidator {
                bottom: 0
                top: 9999
            }
        }
    }

    // 循环次数字段
    ColumnLayout {
        Layout.fillWidth: true
        visible: !isCompactMode

        Label {
            text: qsTr("循环次数")
            color: textColor
        }
        TextField {
            id: recurrenceCountField
            Layout.fillWidth: true
            placeholderText: qsTr("-1表示无限循环")
            color: textColor
            validator: IntValidator {
                bottom: -1
                top: 9999
            }
        }
    }

    // 循环开始日期字段
    ColumnLayout {
        Layout.fillWidth: true
        visible: !isCompactMode

        Label {
            text: qsTr("循环开始日期")
            color: textColor
        }
        TextField {
            id: recurrenceStartDateField
            Layout.fillWidth: true
            placeholderText: qsTr("YYYY-MM-DD")
            color: textColor
        }
    }

    // 公开方法
    function clearForm() {
        titleField.text = "";
        descriptionField.text = "";
        categoryCombo.currentIndex = 0;
        importantCombo.currentIndex = 0;
        deadlineField.text = "";
        recurrenceIntervalField.text = "";
        recurrenceCountField.text = "";
        recurrenceStartDateField.text = "";
    }

    function setFormData(todo) {
        titleField.text = todo.title || "";
        descriptionField.text = todo.description || "";
        categoryCombo.currentIndex = Math.max(0, categoryCombo.model.indexOf(todo.category));
        importantCombo.currentIndex = todo.important ? 1 : 0;
        deadlineField.text = todo.deadline || "";
        recurrenceIntervalField.text = todo.recurrence_interval || "";
        recurrenceCountField.text = todo.recurrence_count || "";
        recurrenceStartDateField.text = todo.recurrence_start_date || "";
    }

    function focusTitle() {
        titleField.forceActiveFocus();
    }

    function getTodoData() {
        return {
            title: titleField.text.trim(),
            description: descriptionField.text.trim(),
            category: categoryCombo.currentText,
            important: importantCombo.currentIndex === 1 // true if "重要", false if "普通"
            ,
            deadline: deadlineField.text.trim(),
            recurrence_interval: recurrenceIntervalField.text.trim() ? parseInt(recurrenceIntervalField.text.trim()) : 0,
            recurrence_count: recurrenceCountField.text.trim() ? parseInt(recurrenceCountField.text.trim()) : -1,
            recurrence_start_date: recurrenceStartDateField.text.trim()
        };
    }

    function isValid() {
        return titleField.text.trim() !== "";
    }
}
