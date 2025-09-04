// 通用标题栏，用于常规页面顶部
import QtQuick
import QtQuick.Layouts
import "../components"

Rectangle {
    id: titleBar
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
        targetWindow: titleBar.targetWindow
    }

    // 左侧返回按钮和标题
    RowLayout {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        IconButton {
            visible: titleBar.showBackButton
            text: "\ue8fa"
            textColor: ThemeManager.textColor
            fontSize: 16
            onClicked: {
                if (titleBar.targetWindow) {
                    titleBar.stackView.pop();
                }
            }
        }

        // 标题
        Text {
            text: titleBar.title
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
            visible: titleBar.showMinimizeButton
            text: "\ue65a"
            onClicked: titleBar.targetWindow.showMinimized()
            textColor: ThemeManager.textColor
            fontSize: 16
        }

        // 最大化/恢复按钮
        IconButton {
            visible: titleBar.showMaximizeButton
            text: titleBar.targetWindow.visibility === Window.Maximized ? "\ue600" : "\ue65b"
            onClicked: {
                if (titleBar.targetWindow.visibility === Window.Maximized) {
                    titleBar.targetWindow.showNormal();
                } else {
                    titleBar.targetWindow.showMaximized();
                }
            }
            textColor: ThemeManager.textColor
            fontSize: 16
        }

        // 关闭按钮
        IconButton {
            visible: titleBar.showCloseButton
            text: "\ue8d1"
            onClicked: titleBar.targetWindow.close()
            textColor: ThemeManager.textColor
            fontSize: 16
        }
    }
}
