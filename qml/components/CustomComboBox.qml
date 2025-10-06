/**
 * @file CustomComboBox.qml
 * @brief 通用下拉框组件
 *
 * 提供统一样式的通用下拉框组件，支持：
 * - 自定义颜色主题
 * - 悬停和按下效果
 * - 可配置的字体大小和圆角
 * - 与项目整体风格一致的外观
 *
 * @author Sakurakugu
 * @date 2025-09-05 00:31:59(UTC+8) 周五
 * @change 2025-09-05 00:31:59(UTC+8) 周五
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

ComboBox {
    id: root

    // 自定义外观属性
    property color textColor: ThemeManager.button2TextColor                  ///< 文本颜色
    property color disabledTextColor: ThemeManager.button2DisabledTextColor  ///< 禁用文本
    property color backgroundColor: ThemeManager.button2Color                ///< 背景颜色
    property color hoverColor: ThemeManager.button2HoverColor                ///< 悬停颜色
    property color pressedColor: ThemeManager.button2PressedColor            ///< 按下颜色
    property color disabledColor: ThemeManager.button2DisabledColor          ///< 禁用颜色
    property color borderColor: ThemeManager.borderColor         ///< 边框颜色
    property int fontSize: 14                                    ///< 字体大小
    property int radius: 4                                       ///< 圆角半径
    property int borderWidth: 1                                  ///< 边框宽度
    property string placeholderText: ""                          ///< 占位文本

    // 尺寸设置
    implicitHeight: 30                                           ///< 默认高度

    // 文本颜色设置
    contentItem: Text {
        leftPadding: 10
        rightPadding: root.indicator.width + 10
        text: root.displayText
        font.pixelSize: root.fontSize
        color: root.enabled ? root.textColor : root.disabledTextColor
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    // 背景设置
    background: Rectangle {
        implicitWidth: 120
        implicitHeight: root.implicitHeight
        color: {
            if (!root.enabled) {
                return root.disabledColor;
            }
            if (root.pressed) {
                return root.pressedColor;
            }
            if (root.hovered) {
                return root.hoverColor;
            }
            return root.backgroundColor;
        }
        border.color: root.borderColor
        border.width: root.borderWidth
        radius: root.radius

        // 颜色过渡动画
        Behavior on color {
            ColorAnimation {
                duration: 150
            }
        }
    }

    // 下拉指示器
    indicator: Canvas {
        id: canvas
        x: root.width - width - 10
        y: (root.height - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = root.enabled ? root.textColor : root.disabledTextColor;
            context.fill();
        }
    }

    // 下拉菜单样式
    popup: Popup {
        y: root.height
        width: root.width
        implicitHeight: contentItem.implicitHeight
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator {}
        }

        background: Rectangle {
            color: root.backgroundColor
            border.color: root.borderColor
            border.width: root.borderWidth
            radius: root.radius
        }
    }

    // 下拉项样式
    delegate: ItemDelegate {
        id: delegateItem
        required property var modelData
        required property int index
        
        width: root.width
        height: 30

        contentItem: Text {
            text: delegateItem.modelData
            color: root.textColor
            font.pixelSize: root.fontSize
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            leftPadding: 10
        }

        background: Rectangle {
            color: delegateItem.highlighted ? root.hoverColor : "transparent"
            radius: root.radius
        }

        highlighted: root.highlightedIndex === delegateItem.index
    }
}
