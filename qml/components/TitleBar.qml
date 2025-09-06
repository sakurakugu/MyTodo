/**
 * @brief 通用标题栏组件
 *
 * 该组件用于创建通用的标题栏，用于常规页面顶部。
 *
 * @author Sakurakugu
 * @date 2025-09-04 23:39:30(UTC+8) 周四
 * @change 2025-09-06 00:46:10(UTC+8) 周六
 * @version 0.4.0
 */
import QtQuick
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    height: 32
    width: parent.width
    color: ThemeManager.secondaryBackgroundColor
    topLeftRadius: 10
    topRightRadius: 10

    property var targetWindow
    property var stackView
    property string title: ""

    property bool showBackButton: false
    property bool showMinimizeButton: true
    property bool showMaximizeButton: true
    property bool showCloseButton: true

    // 窗口拖拽处理区域
    WindowDragHandler {
        anchors.fill: parent
        targetWindow: root.targetWindow
    }

    // 左侧返回按钮和标题
    RowLayout {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        IconButton {
            visible: root.showBackButton
            text: "\ue8fa"
            onClicked: {
                if (root.targetWindow) {
                    root.stackView.pop();
                }
            }
        }

        // 标题
        Text {
            text: root.title
            color: ThemeManager.textColor
            font.pixelSize: 14
            font.bold: true
            Layout.fillWidth: true
            Layout.leftMargin: 10  // 左边空出 10 像素

        }
    }

    // 右侧窗口控制按钮
    RowLayout {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0

        // 最小化按钮
        IconButton {
            visible: root.showMinimizeButton
            text: "\ue65a"
            onClicked: root.targetWindow.showMinimized()
        }

        // 最大化/恢复按钮
        IconButton {
            visible: root.showMaximizeButton
            text: root.targetWindow.visibility === Window.Maximized ? "\ue600" : "\ue65b"
            onClicked: {
                if (root.targetWindow.visibility === Window.Maximized) {
                    root.targetWindow.showNormal();
                } else {
                    root.targetWindow.showMaximized();
                }
            }
        }

        // 关闭按钮
        IconButton {
            visible: root.showCloseButton
            text: "\ue8d1"
            onClicked: root.targetWindow.close()
        }
    }
}
