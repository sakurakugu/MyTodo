/**
 * @file SwitchRow.qml
 * @brief 开关行组件
 *
 * 该文件定义了一个开关行组件，将文本标签放在左侧，开关控件放在右侧，适合在设置页面中使用。
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // 公开属性
    property bool checked: false
    property bool enabled: true
    property string text: ""
    property int spacing: 10

    // 信号
    signal toggled(bool checked)

    // 尺寸设置
    implicitWidth: parent ? parent.width : 200
    implicitHeight: Math.max(switchControl.height, textLabel.implicitHeight)

    // 文本标签
    Text {
        id: textLabel
        text: root.text
        anchors.left: parent.left
        anchors.right: switchControl.left
        anchors.rightMargin: root.spacing
        anchors.verticalCenter: parent.verticalCenter
        color: root.enabled ? ThemeManager.textColor : ThemeManager.secondaryTextColor
        font.pixelSize: 14
        elide: Text.ElideRight
    }

    // 开关控件
    CustomSwitch {
        id: switchControl
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        checked: root.checked
        enabled: root.enabled

        onCheckedChanged: {
            root.checked = checked;
            root.toggled(checked);
        }
    }

    // 鼠标区域 - 允许点击整行切换开关状态
    MouseArea {
        anchors.fill: parent
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            switchControl.toggle();
        }
    }

    // 提供类似于标准Switch的方法
    function toggle() {
        if (root.enabled) {
            switchControl.toggle();
        }
    }
}
