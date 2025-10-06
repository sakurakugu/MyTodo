/**
 * @file ControlRow.qml
 * @brief 通用控件行组件
 *
 * 该文件定义了一个通用的控件行组件，支持多种控件类型（开关、按钮、单选框等），
 * 将文本标签放在左侧，控件放在右侧，适合在设置页面中使用。
 *
 * @author Sakurakugu
 * @date 2025-09-05 18:17:08(UTC+8) 周五
 * @change 2025-09-06 17:22:15(UTC+8) 周六
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Item {
    id: root

    // 控件类型枚举
    enum ControlType {
        Switch,
        Button,
        RadioButton,
        CheckBox,
        ComboBox,
        SpinBox,
        Custom
    }

    // 公开属性
    property int controlType: ControlRow.ControlType.Switch
    property bool checked: false
    property bool enabled: true
    property string icon: ""
    property string text: ""
    property string buttonText: qsTr("按钮")
    property var comboBoxModel: []
    property int currentIndex: 0
    property int spinBoxValue: 0
    property int spinBoxFrom: 0
    property int spinBoxTo: 100
    property int spacing: 10
    property int leftMargin: 0
    property Component customControl: null

    // 信号
    signal toggled(bool checked)
    signal clicked
    signal indexChanged(int index)
    signal valueChanged(int value)
    signal customControlSignal(var data)

    // 尺寸设置
    implicitWidth: parent ? parent.width : 200
    implicitHeight: Math.max(controlLoader.height, textLabel.implicitHeight, 40)

    // 图标标签
    Text {
        id: iconLabel
        text: root.icon
        anchors.left: parent.left
        anchors.leftMargin: root.leftMargin
        anchors.verticalCenter: parent.verticalCenter
        font.family: "iconFont"
        color: root.enabled ? ThemeManager.textColor : ThemeManager.secondaryTextColor
        font.pixelSize: 18
        visible: root.icon !== ""
    }

    // 文本标签
    Text {
        id: textLabel
        text: root.text
        anchors.left: iconLabel.right
        anchors.leftMargin: iconLabel.visible ? 10 : root.leftMargin
        anchors.right: controlLoader.left
        anchors.rightMargin: root.spacing
        anchors.verticalCenter: parent.verticalCenter
        color: root.enabled ? ThemeManager.textColor : ThemeManager.secondaryTextColor
        font.pixelSize: 14
        elide: Text.ElideRight
    }

    // 控件加载器
    Loader {
        id: controlLoader
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        sourceComponent: {
            switch (root.controlType) {
            case ControlRow.ControlType.Switch:
                return switchComponent;
            case ControlRow.ControlType.Button:
                return buttonComponent;
            case ControlRow.ControlType.RadioButton:
                return radioButtonComponent;
            case ControlRow.ControlType.CheckBox:
                return checkBoxComponent;
            case ControlRow.ControlType.ComboBox:
                return comboBoxComponent;
            case ControlRow.ControlType.SpinBox:
                return spinBoxComponent;
            case ControlRow.ControlType.Custom:
                return root.customControl;
            default:
                return switchComponent;
            }
        }
    }

    // 开关组件
    Component {
        id: switchComponent
        CustomSwitch {
            checked: root.checked
            enabled: root.enabled
            onCheckedChanged: {
                root.checked = checked;
                root.toggled(checked);
            }
        }
    }

    // 按钮组件
    Component {
        id: buttonComponent
        CustomButton {
            text: root.buttonText
            enabled: root.enabled
            onClicked: root.clicked()
        }
    }

    // 单选框组件
    Component {
        id: radioButtonComponent
        RadioButton {
            checked: root.checked
            enabled: root.enabled
            onCheckedChanged: {
                root.checked = checked;
                root.toggled(checked);
            }
        }
    }

    // 复选框组件
    Component {
        id: checkBoxComponent
        CustomCheckBox {
            checked: root.checked
            enabled: root.enabled
            onCheckedChanged: {
                root.checked = checked;
                root.toggled(checked);
            }
        }
    }

    // 下拉框组件
    Component {
        id: comboBoxComponent
        CustomComboBox {
            model: root.comboBoxModel
            currentIndex: root.currentIndex
            enabled: root.enabled
            onCurrentIndexChanged: {
                root.currentIndex = currentIndex;
                root.indexChanged(currentIndex);
            }
        }
    }

    // 数字输入框组件
    Component {
        id: spinBoxComponent
        CustomSpinBox {
            value: root.spinBoxValue
            from: root.spinBoxFrom
            to: root.spinBoxTo
            enabled: root.enabled
            onValueChanged: {
                root.spinBoxValue = value;
                root.valueChanged(value);
            }
        }
    }

    // 鼠标区域 - 对于某些控件类型允许点击整行操作
    MouseArea {
        anchors.fill: parent
        enabled: root.enabled && (root.controlType === ControlRow.ControlType.Switch || root.controlType === ControlRow.ControlType.RadioButton || root.controlType === ControlRow.ControlType.CheckBox)
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            root.toggle();
        }
    }

    // 提供通用的操作方法
    // 切换操作 - 开关/复选框/单选框
    function toggle() {
        if (root.enabled && controlLoader.item) {
            if (typeof controlLoader.item.toggle === "function") {
                controlLoader.item.toggle();
            } else if (controlLoader.item.hasOwnProperty("checked")) {
                controlLoader.item.checked = !controlLoader.item.checked;
            }
        }
    }

    // 触发操作 - 按钮点击或开关/复选框/单选框切换
    function triggerAction() {
        if (root.enabled && controlLoader.item) {
            if (root.controlType === ControlRow.ControlType.Button && typeof controlLoader.item.clicked === "function") {
                controlLoader.item.clicked();
            } else {
                toggle();
            }
        }
    }
}
