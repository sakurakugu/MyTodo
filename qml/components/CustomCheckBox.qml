/**
 * @file CustomCheckBox.qml
 * @brief 自定义复选框组件
 *
 * 该文件定义了一个自定义的复选框组件，支持主题切换和动画效果。
 * 提供与项目风格一致的复选框控件。
 *
 * @author Sakurakugu
 * @date 2025-09-05 18:17:08(UTC+8) 周五
 * @change 2025-09-05 18:17:08(UTC+8) 周五
 */
import QtQuick
import QtQuick.Effects

Item {
    id: root

    // 公开属性
    property bool checked: false
    property bool enabled: true
    property string text: ""
    property int textSpacing: 8
    property bool isCheckBoxOnLeft: true // 复选框在左还是右边
    property int checkBoxSize: 18 // 复选框大小
    property int fontSize: 14 // 字体大小
    property int radius: 3 // 圆角半径

    // 信号
    signal toggled(bool checked)

    // 尺寸设置
    implicitWidth: checkBox.width + label.implicitWidth + textSpacing + 4
    implicitHeight: Math.max(checkBox.height, label.implicitHeight)

    // 文本标签（当复选框在右边时显示）
    Text {
        id: leftLabel
        text: root.text
        anchors.right: checkBox.left
        anchors.rightMargin: root.textSpacing
        anchors.verticalCenter: parent.verticalCenter
        color: root.enabled ? ThemeManager.textColor : ThemeManager.secondaryTextColor
        font.pixelSize: root.fontSize
        visible: !root.isCheckBoxOnLeft && root.text !== ""
    }

    // 复选框主体
    Rectangle {
        id: checkBox
        width: root.checkBoxSize
        height: root.checkBoxSize
        radius: root.radius
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: root.isCheckBoxOnLeft ? parent.left : undefined
        anchors.right: !root.isCheckBoxOnLeft ? parent.right : undefined

        color: {
            if (!root.enabled) {
                return ThemeManager.buttonDisabledColor;
            }
            return root.checked ? ThemeManager.successColor : ThemeManager.backgroundColor;
        }

        border.width: root.checked ? 0 : 1
        border.color: root.enabled ? ThemeManager.borderColor : ThemeManager.button2DisabledTextColor

        // 过渡动画
        Behavior on color {
            ColorAnimation {
                duration: 150
            }
        }

        Behavior on border.width {
            NumberAnimation {
                duration: 150
            }
        }

        // 复选框阴影
        layer.enabled: root.enabled
        layer.effect: MultiEffect {
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 1
            shadowBlur: 1.0
            shadowColor: ThemeManager.shadowColor
        }

        // 勾选标记
        Item {
            id: checkMark
            anchors.centerIn: parent
            width: parent.width * 0.6
            height: parent.height * 0.6
            opacity: root.checked ? 1.0 : 0.0
            scale: root.checked ? 1.0 : 0.5

            // 透明度和缩放动画
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.InOutQuad
                }
            }

            Behavior on scale {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.OutBack
                }
            }

            // 勾选图标路径
            Canvas {
                id: checkIcon
                anchors.fill: parent

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);

                    if (root.checked) {
                        ctx.strokeStyle = "white";
                        ctx.lineWidth = 2;
                        ctx.lineCap = "round";
                        ctx.lineJoin = "round";

                        // 绘制勾选标记
                        ctx.beginPath();
                        ctx.moveTo(width * 0.2, height * 0.5);
                        ctx.lineTo(width * 0.45, height * 0.75);
                        ctx.lineTo(width * 0.8, height * 0.25);
                        ctx.stroke();
                    }
                }
            }
        }
    }

    // 文本标签（当复选框在左边时显示）
    Text {
        id: label
        text: root.text
        anchors.left: checkBox.right
        anchors.leftMargin: root.textSpacing
        anchors.verticalCenter: parent.verticalCenter
        color: root.enabled ? ThemeManager.textColor : ThemeManager.secondaryTextColor
        font.pixelSize: root.fontSize
        visible: root.isCheckBoxOnLeft && root.text !== ""
    }

    // 鼠标区域
    MouseArea {
        anchors.fill: parent
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onClicked: {
            root.checked = !root.checked;
            root.toggled(root.checked);
        }

        onEntered: {
            checkBox.scale = 1.05;
        }

        onExited: {
            checkBox.scale = 1.0;
        }

        // 缩放动画
        Behavior on scale {
            NumberAnimation {
                duration: 100
                easing.type: Easing.InOutQuad
            }
        }
    }

    // 提供类似于标准CheckBox的方法
    function toggle() {
        if (root.enabled) {
            checked = !checked;
            toggled(checked);
        }
    }

    // 重绘勾选图标
    onCheckedChanged: {
        checkIcon.requestPaint();
    }

    Component.onCompleted: {
        checkIcon.requestPaint();
    }
}
