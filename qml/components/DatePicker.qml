/**
 * @file DatePicker.qml
 * @brief 日期选择器组件
 *
 * 提供日期选择功能的对话框组件，支持：
 * - 年月日选择
 * - 主题适配
 * - 确认和取消操作
 *
 * @author Sakurakugu
 * @date 2025-01-XX
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: datePickerDialog
    
    property date selectedDate: new Date()
    property int selectedYear: selectedDate.getFullYear()
    property int selectedMonth: selectedDate.getMonth() + 1
    property int selectedDay: selectedDate.getDate()
    
    title: "选择日期"
    modal: true
    width: 350
    height: 300
    
    // 居中显示
    anchors.centerIn: parent
    
    background: Rectangle {
        color: theme.backgroundColor
        border.color: theme.borderColor
        border.width: 1
        radius: 8
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16
        
        // 标题
        Text {
            text: "选择日期"
            font.pixelSize: 16
            font.bold: true
            color: theme.textColor
            Layout.alignment: Qt.AlignHCenter
        }
        
        // 日期选择区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 4
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                
                // 年份选择
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    Text {
                        text: "年份:"
                        color: theme.textColor
                        font.pixelSize: 14
                        Layout.preferredWidth: 50
                    }
                    
                    SpinBox {
                        id: yearSpinBox
                        from: 1900
                        to: 2100
                        value: datePickerDialog.selectedYear
                        Layout.fillWidth: true
                        
                        onValueChanged: {
                            datePickerDialog.selectedYear = value
                            updateSelectedDate()
                        }
                    }
                }
                
                // 月份选择
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    Text {
                        text: "月份:"
                        color: theme.textColor
                        font.pixelSize: 14
                        Layout.preferredWidth: 50
                    }
                    
                    SpinBox {
                        id: monthSpinBox
                        from: 1
                        to: 12
                        value: datePickerDialog.selectedMonth
                        Layout.fillWidth: true
                        
                        onValueChanged: {
                            datePickerDialog.selectedMonth = value
                            updateSelectedDate()
                        }
                    }
                }
                
                // 日期选择
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    Text {
                        text: "日期:"
                        color: theme.textColor
                        font.pixelSize: 14
                        Layout.preferredWidth: 50
                    }
                    
                    SpinBox {
                        id: daySpinBox
                        from: 1
                        to: getDaysInMonth(datePickerDialog.selectedYear, datePickerDialog.selectedMonth)
                        value: datePickerDialog.selectedDay
                        Layout.fillWidth: true
                        
                        onValueChanged: {
                            datePickerDialog.selectedDay = value
                            updateSelectedDate()
                        }
                    }
                }
            }
        }
        
        // 当前选择显示
        Text {
            text: "选择的日期: " + Qt.formatDate(datePickerDialog.selectedDate, "yyyy-MM-dd")
            color: theme.textColor
            font.pixelSize: 14
            Layout.alignment: Qt.AlignHCenter
        }
        
        // 按钮区域
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Item {
                Layout.fillWidth: true
            }
            
            Button {
                text: "取消"
                onClicked: {
                    datePickerDialog.rejected()
                    datePickerDialog.close()
                }
                
                background: Rectangle {
                    color: parent.pressed ? (globalState.isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (globalState.isDarkMode ? "#3c5a78" : "#e0e0e0") : (globalState.isDarkMode ? "#2c3e50" : "#f0f0f0")
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            
            Button {
                text: "确定"
                onClicked: {
                    datePickerDialog.accepted()
                    datePickerDialog.close()
                }
                
                background: Rectangle {
                    color: parent.pressed ? "#005A9E" : parent.hovered ? "#0078D4" : "#007ACC"
                    border.color: "#007ACC"
                    border.width: 1
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
    
    // 获取指定年月的天数
    function getDaysInMonth(year, month) {
        return new Date(year, month, 0).getDate()
    }
    
    // 更新选择的日期
    function updateSelectedDate() {
        // 确保日期有效
        var maxDay = getDaysInMonth(selectedYear, selectedMonth)
        if (selectedDay > maxDay) {
            selectedDay = maxDay
            daySpinBox.value = selectedDay
        }
        
        selectedDate = new Date(selectedYear, selectedMonth - 1, selectedDay)
    }
    
    // 当selectedDate改变时更新SpinBox的值
    onSelectedDateChanged: {
        selectedYear = selectedDate.getFullYear()
        selectedMonth = selectedDate.getMonth() + 1
        selectedDay = selectedDate.getDate()
        
        yearSpinBox.value = selectedYear
        monthSpinBox.value = selectedMonth
        daySpinBox.value = selectedDay
    }
}