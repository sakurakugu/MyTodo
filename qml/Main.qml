import QtQuick
import QtQuick.Controls

Window {
    id: root
    visible: true
    width: 800
    height: 640
    minimumWidth: 800
    minimumHeight: 640
    title: qsTr("我的待办")

    // 背景透明度设置 - 必须保持透明，否则在Windows下会出现黑色背景问题
    color: "transparent"

    // 窗口标志设置 - FramelessWindowHint必须始终存在，否则Windows下会出现背景变黑且无法恢复的问题

    flags: Qt.FramelessWindowHint | (globalState.isDesktopWidget ? Qt.Tool : Qt.Window) | Qt.WindowStaysOnTopHint

    // 字体加载器
    FontLoader {
        id: iconFont
        source: "qrc:/qt/qml/MyTodo/image/font_icon/iconfont.ttf"
    }

    // 登录相关对话框组件
    LoginStatusDialogs {
        id: loginStatusDialogs
    }

    // 分类管理器
    TodoCategoryManager {
        id: todoCategoryManager
    }

    // 主题管理器
    readonly property var theme: ThemeManager          ///< 主题

    // 主窗口背景容器
    Rectangle {
        id: mainBackground
        anchors.fill: parent
        anchors.margins: 0
        visible: !globalState.isDesktopWidget
        color: theme.backgroundColor
        radius: 5                        ///< 窗口圆角半径
        border.color: theme.borderColor
        border.width: 1
        z: -10                          ///< 确保背景在所有内容下方
    }

    // 导航栈，进行页面管理
    StackView {
        id: stackView
        anchors.fill: parent
        anchors.margins: 0
        z: 1
        focus: true
        visible: !globalState.isDesktopWidget && depth > 0 // 小窗口模式时隐藏主页面
        initialItem: homePage
        clip: true  ///< 裁剪内容以配合窗口圆角效果
    }

    // 各种页面
    // 主页面
    HomePage {
        id: homePage
        root: root
        stackView: stackView
        loginStatusDialogs: loginStatusDialogs
        todoCategoryManager: todoCategoryManager
    }

    WidgetMode {
        id: widgetMode
        visible: globalState.isDesktopWidget
        loginStatusDialogs: loginStatusDialogs
        todoCategoryManager: todoCategoryManager
    }

    // 边框调整大小区域
    property int resizeBorderWidth: 5     ///< 边框调整大小的边距宽度
    // 左边框
    MouseArea {
        id: leftResizeArea
        width: root.resizeBorderWidth
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: !globalState.isDesktopWidget
        cursorShape: Qt.SizeHorCursor
        z: 2000

        property string edge: "left"

        onPressed: {
            root.startSystemResize(Qt.LeftEdge);
        }
    }

    // 右边框
    MouseArea {
        id: rightResizeArea
        width: root.resizeBorderWidth
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: !globalState.isDesktopWidget
        cursorShape: Qt.SizeHorCursor
        z: 2000

        property string edge: "right"

        onPressed: {
            root.startSystemResize(Qt.RightEdge);
        }
    }

    // 上边框
    MouseArea {
        id: topResizeArea
        height: root.resizeBorderWidth
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        visible: !globalState.isDesktopWidget
        cursorShape: Qt.SizeVerCursor
        z: 2000

        property string edge: "top"

        onPressed: {
            root.startSystemResize(Qt.TopEdge);
        }
    }

    // 下边框
    MouseArea {
        id: bottomResizeArea
        height: root.resizeBorderWidth
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: !globalState.isDesktopWidget
        cursorShape: Qt.SizeVerCursor
        z: 2000

        property string edge: "bottom"

        onPressed: {
            root.startSystemResize(Qt.BottomEdge);
        }
    }

    // 左上角
    MouseArea {
        id: topLeftResizeArea
        width: root.resizeBorderWidth
        height: root.resizeBorderWidth
        anchors.left: parent.left
        anchors.top: parent.top
        visible: !globalState.isDesktopWidget
        cursorShape: Qt.SizeFDiagCursor
        z: 2000

        property string edge: "topleft"

        onPressed: {
            root.startSystemResize(Qt.TopEdge | Qt.LeftEdge);
        }
    }

    // 右上角
    MouseArea {
        id: topRightResizeArea
        width: root.resizeBorderWidth
        height: root.resizeBorderWidth
        anchors.right: parent.right
        anchors.top: parent.top
        visible: !globalState.isDesktopWidget
        cursorShape: Qt.SizeBDiagCursor
        z: 2000

        property string edge: "topright"

        onPressed: {
            root.startSystemResize(Qt.TopEdge | Qt.RightEdge);
        }
    }

    // 左下角
    MouseArea {
        id: bottomLeftResizeArea
        width: root.resizeBorderWidth
        height: root.resizeBorderWidth
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        visible: !globalState.isDesktopWidget
        cursorShape: Qt.SizeBDiagCursor
        z: 2000

        property string edge: "bottomleft"

        onPressed: {
            root.startSystemResize(Qt.BottomEdge | Qt.LeftEdge);
        }
    }

    // 右下角
    MouseArea {
        id: bottomRightResizeArea
        width: root.resizeBorderWidth
        height: root.resizeBorderWidth
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: !globalState.isDesktopWidget
        cursorShape: Qt.SizeFDiagCursor
        z: 2000

        property string edge: "bottomright"

        onPressed: {
            root.startSystemResize(Qt.BottomEdge | Qt.RightEdge);
        }
    }
}
