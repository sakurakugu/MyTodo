/**
 * @file TodoDetailsDialog.qml
 * @brief 待办详情对话框组件
 *
 * 该文件定义了一个待办详情对话框组件，用于显示和编辑待办事项的详细信息。
 * 它包含一个任务表单组件，用于输入和编辑待办事项的标题、描述、截止日期等信息。
 *
 * @author Sakurakugu
 * @date 2025-08-19 07:39:54(UTC+8) 周二
 * @version 2025-08-21 21:31:41(UTC+8) 周四
 */
 
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    id: todoDetailsDialog
    dialogTitle: qsTr("待办详情")
    dialogWidth: 400
    dialogHeight: Math.min(parent ? parent.height * 0.8 : 500, 500)
    showStandardButtons: true
    standardButtons: Dialog.Ok | Dialog.Cancel

    property bool isDarkMode: false
    property var currentTodo: null
    property int currentIndex: -1
    
    // 信号定义
    signal todoUpdated(int index, var todoData)

    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: todoDetailsDialog.isDarkMode
    }

    function openTodoDetails(todo, index) {
        currentTodo = todo;
        currentIndex = index;
        taskForm.setFormData(todo);
        open();
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: 400
        clip: true
        
        TaskForm {
            id: taskForm
            width: parent.width
            theme: todoDetailsDialog.theme
        }
    }

    onAccepted: {
        if (currentTodo && taskForm.isValid()) {
            var todoData = taskForm.getTodoData();
            todoUpdated(currentIndex, todoData);
        }
    }

    onRejected: {
        // 取消时重置表单
        if (currentTodo) {
            taskForm.setFormData(currentTodo);
        }
    }
}