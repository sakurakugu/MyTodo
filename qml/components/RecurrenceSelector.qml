/**
 * @file RecurrenceSelector.qml
 * @brief 重复间隔选择器组件
 *
 * 提供离散值选择的重复间隔设置组件，支持：
 * - 预设选项：∞（-999）、每年（-365）、每月（-30）、每天（-1或1）
 * - 自定义天数输入
 * - 与项目整体风格一致的外观
 *
 * @author Sakurakugu
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    spacing: 8

    // 公开属性
    property int value: 0                                        ///< 当前值
    property bool enabled: true                                  ///< 是否启用
    property color textColor: ThemeManager.textColor            ///< 文本颜色
    property color backgroundColor: ThemeManager.button2Color    ///< 背景颜色
    property color hoverColor: ThemeManager.button2HoverColor   ///< 悬停颜色
    property color borderColor: ThemeManager.borderColor        ///< 边框颜色

    // 信号
    signal intervalChanged(int newValue)

    // 预设选项数据
    property var presetOptions: [
        { text: qsTr("不重复"), value: 0, description: qsTr("永不重复") },
        { text: qsTr("每天"), value: -1, description: qsTr("每天重复") },
        { text: qsTr("每周"), value: -7, description: qsTr("每周重复") },
        { text: qsTr("每月"), value: -30, description: qsTr("每月重复") },
        { text: qsTr("每年"), value: -365, description: qsTr("每年重复") },
        { text: qsTr("永远"), value: -999, description: qsTr("一直重复") },
    ]

    // 内部状态
    property bool isCustomMode: {
        for (let i = 0; i < presetOptions.length; i++) {
            if (presetOptions[i].value === value) {
                return false;
            }
        }
        return value > 0;
    }

    // 获取当前显示文本
    function getCurrentDisplayText() {
        for (let i = 0; i < presetOptions.length; i++) {
            if (presetOptions[i].value === value) {
                return presetOptions[i].text;
            }
        }
        if (value > 0) {
            return value.toString();
        }
        return "0";
    }

    // 获取当前描述
    function getCurrentDescription() {
        for (let i = 0; i < presetOptions.length; i++) {
            if (presetOptions[i].value === value) {
                return presetOptions[i].description;
            }
        }
        if (value > 0) {
            return qsTr("每") + value + qsTr("天重复");
        }
        return qsTr("不重复");
    }

    // 下拉选择框
    CustomComboBox {
        id: comboBox
        Layout.preferredWidth: 90
        Layout.preferredHeight: 25
        enabled: root.enabled

        // 构建模型
        model: {
            let items = [];
            // 添加预设选项
            for (let i = 0; i < presetOptions.length; i++) {
                items.push(presetOptions[i].text);
            }
            // 添加自定义选项
            items.push(qsTr("自定义"));
            return items;
        }

        // 设置当前索引
        currentIndex: {
            for (let i = 0; i < presetOptions.length; i++) {
                if (presetOptions[i].value === root.value) {
                    return i;
                }
            }
            // 如果是自定义值（大于0），选择"自定义"选项
            if (root.value >= 0) {
                return presetOptions.length; // "自定义"选项的索引
            }
            return 0; // 默认选择第一个选项
        }

        onCurrentIndexChanged: {
            // 防止绑定循环：只有当用户主动选择时才更新值
            if (currentIndex >= 0 && currentIndex < presetOptions.length) {
                // 选择了预设选项
                let newValue = presetOptions[currentIndex].value;
                if (newValue !== root.value) {
                    // 临时断开绑定以避免循环
                    Qt.callLater(function () {
                        root.value = newValue;
                        root.intervalChanged(newValue);
                    });
                }
            } else if (currentIndex === presetOptions.length) {
                // 选择了"自定义"选项
                if (root.value <= 0) {
                    Qt.callLater(function () {
                        root.value = 1;
                        root.intervalChanged(1);
                    });
                }
            }
        }
    }

    // 自定义数值输入框（仅在自定义模式下显示）
    CustomSpinBox {
        id: customSpinBox
        Layout.preferredWidth: 60
        Layout.preferredHeight: 25
        visible: root.isCustomMode
        enabled: root.enabled && root.isCustomMode
        from: 0
        to: 365
        value: root.value >= 0 ? root.value : 0
        stepSize: 1

        onValueChanged: {
            if (root.isCustomMode && value !== root.value) {
                root.value = value;
                root.intervalChanged(value);
            }
        }
    }

    // 描述文本
    Text {
        text: root.isCustomMode ? qsTr("天重复") : getCurrentDescription()
        font.pixelSize: 16
        color: root.textColor
        verticalAlignment: Text.AlignVCenter
        Layout.fillWidth: true
    }

    // 监听value变化，更新内部状态
    onValueChanged: {
        // 确保comboBox的currentIndex正确
        let foundIndex = -1;
        for (let i = 0; i < presetOptions.length; i++) {
            if (presetOptions[i].value === value) {
                foundIndex = i;
                break;
            }
        }

        if (foundIndex >= 0) {
            if (comboBox.currentIndex !== foundIndex) {
                comboBox.currentIndex = foundIndex;
            }
        } else if (value > 0) {
            if (comboBox.currentIndex !== presetOptions.length) {
                comboBox.currentIndex = presetOptions.length;
            }
            if (customSpinBox.value !== value) {
                customSpinBox.value = value;
            }
        }
    }
}
