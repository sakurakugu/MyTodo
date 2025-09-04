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

    // 背景透明度设置 - 必须保持透明，否则在Windows下的小窗口模式下会出现黑色背景问题
    color: "transparent"
    // 窗口标志设置 - 必须保持透明，否则在Windows下的小窗口模式下会出现黑色背景问题
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

    // 主窗口背景容器
    Rectangle {
        id: mainBackground
        anchors.fill: parent
        anchors.margins: 0
        visible: !globalState.isDesktopWidget
        color: ThemeManager.backgroundColor
        radius: 10                        ///< 窗口圆角半径
        border.color: ThemeManager.borderColor
        border.width: 1
        z: -10                           ///< 确保背景在所有内容下方
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
        // initialItem: testPage
        clip: true  ///< 裁剪内容以配合窗口圆角效果
    }

    // 各种页面
    // 测试页面
    TestPage {
        id: testPage
        root: root
    }

    // 主页面
    HomePage {
        id: homePage
        root: root
        stackView: stackView
        loginStatusDialogs: loginStatusDialogs
        todoCategoryManager: todoCategoryManager
    }

    // 小组件模式
    WidgetMode {
        id: widgetMode
        visible: globalState.isDesktopWidget
        loginStatusDialogs: loginStatusDialogs
        todoCategoryManager: todoCategoryManager
    }

    // 窗口边框调整大小处理组件
    WindowResizeHandler {
        id: windowResizeHandler
        anchors.fill: parent
        borderWidth: 5
        enabled: !globalState.isDesktopWidget
        targetWindow: root
    }
}
