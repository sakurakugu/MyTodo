/**
 * @file TodoDetailsDialog.qml
 * @brief 待办详情对话框组件
 *
 * 该文件定义了一个待办详情对话框组件，用于显示和编辑待办事项的详细信息。
 * 它包含一个任务表单组件，用于输入和编辑待办事项的标题、描述、截止日期等信息。
 *
 * @author Sakurakugu
 * @date 2025
 */
 
import QtQuick
import QtQuick.Controls

Dialog {
    id: todoDetailsDialog
    title: qsTr("待办详情")
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: 400
    height: Math.min(parent ? parent.height * 0.8 : 500, 500)
    modal: true
    anchors.centerIn: parent

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

    background: Rectangle {
        color: theme.backgroundColor
        border.color: theme.borderColor
        border.width: 1
        radius: 8
    }

    ScrollView {
        anchors.fill: parent
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