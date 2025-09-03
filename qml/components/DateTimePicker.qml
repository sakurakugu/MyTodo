/**
 * @file DatePicker.qml
 * @brief 日期时间选择器组件
 *
 * 提供日期和时间选择功能的对话框组件，支持：
 * - Calendar日历选择
 * - 时间选择
 * - 日期/时间模式切换
 * - 主题适配
 * - 确认和取消操作
 *
 * @author Sakurakugu
 * @date 2025-01-XX
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

Dialog {
    id: root

    property date selectedDate: new Date()                  // 选中的日期
    property int selectedHour: selectedDate.getHours()      // 选中的小时
    property int selectedMinute: selectedDate.getMinutes()  // 选中的分钟
    property int selectedSecond: selectedDate.getSeconds()  // 选中的秒
    property bool enableTimeMode: true                      // 是否开启时间模式
    property bool isTimeMode: false                         // false: 日期模式, true: 时间模式
    property real animationDuration: 200                    // 动画持续时间

    // title: isTimeMode ? "选择时间" : "选择日期"
    modal: true
    width: 400
    height: isTimeMode ? 350 : 450

    // 居中显示
    anchors.centerIn: parent

    // 主题管理器
    readonly property var theme: ThemeManager          ///< 主题

    header: Rectangle {
        color: theme.backgroundColor
        border.color: theme.borderColor
        height: 56
        topLeftRadius: 10                                     ///< 左上角圆角
        topRightRadius: 10                                    ///< 右上角圆角

        // 顶部模式切换
        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: root.enableTimeMode ? (!root.isTimeMode ? theme.selectedColor : theme.backgroundColor) : theme.backgroundColor
                radius: 8

                Text {
                    id: dateText
                    text: root.enableTimeMode ? "日期" : "选择日期"
                    color: theme.textColor
                    font.pixelSize: 16
                    font.bold: !root.isTimeMode
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.isTimeMode = false;
                        contentLayout.enableYearMonthMode = false;
                    }
                }
            }

            Rectangle {
                visible: root.enableTimeMode
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: root.isTimeMode ? theme.selectedColor : theme.backgroundColor
                radius: 8

                Text {
                    id: timeText
                    text: "时间"
                    color: theme.textColor
                    font.pixelSize: 16
                    font.bold: root.isTimeMode
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.isTimeMode = true;
                        contentLayout.enableYearMonthMode = false;
                    }
                }
            }
        }

        // 底部分割线
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: theme.borderColor
        }
    }

    background: Rectangle {
        color: theme.backgroundColor
        border.color: theme.borderColor
        border.width: 1
        radius: 8

        // 添加阴影效果
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 2
            shadowBlur: 0.8
            shadowColor: theme.shadowColor
        }
    }

    contentItem: ColumnLayout {
        id: contentLayout
        anchors.margins: 16
        spacing: 16

        property bool enableYearMonthMode: false

        // 日期选择区域 (Calendar)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 4
            visible: !root.isTimeMode && !contentLayout.enableYearMonthMode

            GridLayout {
                id: calendarGrid
                anchors.fill: parent
                anchors.margins: 8
                columns: 7
                rows: 7

                property date currentDate: root.selectedDate                                   // 当前日期
                property int currentYear: currentDate.getFullYear()                            // 当前年份
                property int currentMonth: currentDate.getMonth()                              // 当前月份
                property date firstDayOfMonth: new Date(currentYear, currentMonth, 1)          // 本月第一天
                property int firstDayWeekday: firstDayOfMonth.getDay()                         // 本月第一天是周几
                property int daysInMonth: new Date(currentYear, currentMonth + 1, 0).getDate() // 本月有多少天

                // 月份导航
                RowLayout {
                    Layout.columnSpan: 7    // 占满7列
                    Layout.fillWidth: true  // 占满宽度

                    Button {
                        text: "<"
                        onClicked: {
                            var newDate = new Date(calendarGrid.currentYear, calendarGrid.currentMonth - 1, 1);
                            calendarGrid.currentDate = newDate;
                            calendarGrid.updateCalendar();
                        }

                        background: Rectangle {
                            color: parent.pressed ? theme.borderColor : "transparent"
                            radius: 4
                        }

                        contentItem: Text {
                            text: parent.text
                            color: theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        color: "transparent"

                        Text {
                            id: yearMonthText
                            text: Qt.formatDate(calendarGrid.currentDate, "yyyy年MM月")
                            color: theme.textColor
                            font.pixelSize: 14
                            font.bold: true
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                contentLayout.enableYearMonthMode = true;
                            }
                        }
                    }

                    Button {
                        text: ">"
                        onClicked: {
                            var newDate = new Date(calendarGrid.currentYear, calendarGrid.currentMonth + 1, 1);
                            calendarGrid.currentDate = newDate;
                            calendarGrid.updateCalendar();
                        }

                        background: Rectangle {
                            color: parent.pressed ? theme.borderColor : "transparent"
                            radius: 4
                        }

                        contentItem: Text {
                            text: parent.text
                            color: theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                // 星期标题
                Repeater {
                    model: ["日", "一", "二", "三", "四", "五", "六"]

                    Text {
                        text: modelData
                        color: theme.textColor
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                    }
                }

                // 日期按钮
                Repeater {
                    id: dayRepeater
                    model: 42 // 6周 x 7天

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 35

                        property int dayNumber: {
                            var dayIndex = index - calendarGrid.firstDayWeekday;
                            return dayIndex + 1;
                        }

                        property bool isValidDay: dayNumber > 0 && dayNumber <= calendarGrid.daysInMonth
                        property bool isSelected: {
                            if (!isValidDay)
                                return false;
                            var selectedDay = root.selectedDate.getDate();
                            var selectedMonth = root.selectedDate.getMonth();
                            var selectedYear = root.selectedDate.getFullYear();
                            return dayNumber === selectedDay && calendarGrid.currentMonth === selectedMonth && calendarGrid.currentYear === selectedYear;
                        }
                        property bool isToday: {
                            if (!isValidDay)
                                return false;
                            var today = new Date();
                            return dayNumber === today.getDate() && calendarGrid.currentMonth === today.getMonth() && calendarGrid.currentYear === today.getFullYear();
                        }

                        color: {
                            if (isSelected)
                                return "#007ACC";
                            if (isToday)
                                return theme.borderColor;
                            return "transparent";
                        }
                        border.color: isToday ? "#007ACC" : "transparent"
                        border.width: isToday ? 1 : 0
                        radius: 4

                        Text {
                            anchors.centerIn: parent
                            text: parent.isValidDay ? parent.dayNumber : ""
                            color: {
                                if (parent.isSelected)
                                    return "white";
                                if (!parent.isValidDay)
                                    return "transparent";
                                return theme.textColor;
                            }
                            font.pixelSize: 12
                            font.bold: parent.isSelected || parent.isToday
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: parent.isValidDay

                            onClicked: {
                                root.selectedDate = new Date(calendarGrid.currentYear, calendarGrid.currentMonth, parent.dayNumber, root.selectedHour, root.selectedMinute, root.selectedSecond);
                            }
                        }
                    }
                }

                function updateCalendar() {
                    currentYear = currentDate.getFullYear();
                    currentMonth = currentDate.getMonth();
                    firstDayOfMonth = new Date(currentYear, currentMonth, 1);
                    firstDayWeekday = firstDayOfMonth.getDay();
                    daysInMonth = new Date(currentYear, currentMonth + 1, 0).getDate();
                }

                Component.onCompleted: updateCalendar()
            }
        }

        // 时间选择区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 4
            visible: root.isTimeMode

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 16

                Text {
                    text: "设置时间"
                    font.pixelSize: 16
                    font.bold: true
                    color: theme.textColor
                    Layout.alignment: Qt.AlignHCenter
                }

                // 时间滚动选择器
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 20

                    // 小时选择器
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 8

                        Text {
                            text: "小时"
                            color: theme.textColor
                            font.pixelSize: 14
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Tumbler {
                            id: hourTumbler
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: 120
                            currentIndex: root.selectedHour
                            model: 24
                            visibleItemCount: 5

                            delegate: Text {
                                text: String(modelData).padStart(2, '0')
                                color: theme.textColor
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
                            }

                            onCurrentIndexChanged: {
                                root.selectedHour = currentIndex;
                                updateSelectedDateTime();
                            }
                        }
                    }

                    // 分钟选择器
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 8

                        Text {
                            text: "分钟"
                            color: theme.textColor
                            font.pixelSize: 14
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Tumbler {
                            id: minuteTumbler
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: 120
                            currentIndex: root.selectedMinute
                            model: 60
                            visibleItemCount: 5

                            delegate: Text {
                                text: String(modelData).padStart(2, '0')
                                color: theme.textColor
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
                            }

                            onCurrentIndexChanged: {
                                root.selectedMinute = currentIndex;
                                updateSelectedDateTime();
                            }
                        }
                    }

                    // 秒选择器
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 8

                        Text {
                            text: "秒"
                            color: theme.textColor
                            font.pixelSize: 14
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Tumbler {
                            id: secondTumbler
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: 120
                            currentIndex: root.selectedSecond
                            model: 60
                            visibleItemCount: 5

                            delegate: Text {
                                text: String(modelData).padStart(2, '0')
                                color: theme.textColor
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
                            }

                            onCurrentIndexChanged: {
                                root.selectedSecond = currentIndex;
                                updateSelectedDateTime();
                            }
                        }
                    }
                }
            }
        }

        // 年月选择器
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 4
            visible: !root.isTimeMode && contentLayout.enableYearMonthMode

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                    color: "transparent"

                    Text {
                        text: "选择年月"
                        color: theme.textColor
                        font.pixelSize: 16
                        font.bold: true
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            contentLayout.enableYearMonthMode = false;
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 18

                    // 年份选择
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "年份"
                            color: theme.textColor
                            font.pixelSize: 14
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Tumbler {
                            id: yearTumbler
                            Layout.fillWidth: true
                            Layout.preferredHeight: 100
                            model: {
                                var years = [];
                                var currentYear = new Date().getFullYear();
                                for (var i = currentYear - 50; i <= currentYear + 50; i++) {
                                    years.push(i);
                                }
                                return years;
                            }
                            currentIndex: {
                                var currentYear = calendarGrid.currentYear;
                                var startYear = new Date().getFullYear() - 50;
                                return currentYear - startYear;
                            }
                        }
                    }

                    // 月份选择
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "月份"
                            color: theme.textColor
                            font.pixelSize: 14
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Tumbler {
                            id: monthTumbler
                            Layout.fillWidth: true
                            Layout.preferredHeight: 100
                            model: ["1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"]
                            currentIndex: calendarGrid.currentMonth
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        text: "取消"
                        Layout.fillWidth: true
                        onClicked: {
                            contentLayout.enableYearMonthMode = false;
                        }

                        background: Rectangle {
                            color: parent.pressed ? theme.borderColor : "transparent"
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
                        Layout.fillWidth: true
                        onClicked: {
                            var selectedYear = yearTumbler.model[yearTumbler.currentIndex];
                            var selectedMonth = monthTumbler.currentIndex;
                            calendarGrid.currentDate = new Date(selectedYear, selectedMonth, 1);
                            calendarGrid.updateCalendar();
                            contentLayout.enableYearMonthMode = false;
                        }

                        background: Rectangle {
                            color: parent.pressed ? "#005A9E" : parent.hovered ? "#0078D4" : "#007ACC"
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
                    root.rejected();
                    root.close();
                }

                background: Rectangle {
                    color: parent.pressed ? (globalState.isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (globalState.isDarkMode ? "#3c5a78" : "#e0e0e0") : (globalState.isDarkMode ? "#2c3e50" : "#f0f0f0")
                    border.color: theme.borderColor
                    border.width: 1
                    radius: 8
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
                    root.accepted();
                    root.close();
                }

                background: Rectangle {
                    color: parent.pressed ? "#005A9E" : parent.hovered ? "#0078D4" : "#007ACC"
                    border.color: "#007ACC"
                    border.width: 1
                    radius: 8
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

    // 更新选择的日期时间
    function updateSelectedDateTime() {
        selectedDate = new Date(selectedDate.getFullYear(), selectedDate.getMonth(), selectedDate.getDate(), selectedHour, selectedMinute, selectedSecond);
    }

    // 当selectedDate改变时更新时间相关的值
    onSelectedDateChanged: {
        selectedHour = selectedDate.getHours();
        selectedMinute = selectedDate.getMinutes();
        selectedSecond = selectedDate.getSeconds();

        // 更新时间Tumbler的值（如果存在）
        if (hourTumbler)
            hourTumbler.currentIndex = selectedHour;
        if (minuteTumbler)
            minuteTumbler.currentIndex = selectedMinute;
        if (secondTumbler)
            secondTumbler.currentIndex = selectedSecond;

        // 更新自定义日历的当前日期
        if (calendarGrid) {
            calendarGrid.currentDate = selectedDate;
            calendarGrid.updateCalendar();
        }
    }

    // 进入动画
    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: root.animationDuration
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                property: "scale"
                from: 0.8
                to: 1.0
                duration: root.animationDuration
                easing.type: Easing.OutBack
                easing.overshoot: 1.2
            }
        }
    }

    // 退出动画
    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: root.animationDuration * 0.75
                easing.type: Easing.InQuad
            }
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.9
                duration: root.animationDuration * 0.75
                easing.type: Easing.InQuad
            }
        }
    }
}
