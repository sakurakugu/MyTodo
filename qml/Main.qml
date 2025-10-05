/**
 * @file Main.qml
 * @brief 主应用组件
 *
 * 该组件是应用的入口点，负责创建和管理应用的窗口、页面和组件。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-06 17:22:15(UTC+8) 周六
 */
import QtQuick
import QtQuick.Controls

Window {
    id: root
    visible: true
    width: globalState.isDesktopWidget ? 400 : 800 // 400是小组件的宽度
    height: globalState.isDesktopWidget ? 35 : 640 // 35是小组件标题栏的高度
    minimumWidth: globalState.isDesktopWidget ? 400 : 800
    minimumHeight: globalState.isDesktopWidget ? 35 : 640
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
    MainBackground {
        visible: !globalState.isDesktopWidget
    }

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.margins: 0
        z: 1
        focus: true
        visible: !globalState.isDesktopWidget && depth > 0 // 小窗口模式时隐藏主页面
        initialItem: homePage
        clip: true  ///< 裁剪内容以配合窗口圆角效果

        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
            }
        }
        pushExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 200
            }
        }
        popEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
            }
        }
        popExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 200
            }
        }
    }

    // 各种页面
    // 主页面
    HomePage {
        id: homePage
        root: root
        stackView: stackView
        settingPage: settingPage
        loginStatusDialogs: loginStatusDialogs
        todoCategoryManager: todoCategoryManager
    }

    // 设置页面
    SettingPage {
        id: settingPage
        root: root
        stackView: stackView
        loginStatusDialogs: loginStatusDialogs
    }

    // 小组件模式
    WidgetMode {
        id: widgetMode
        visible: globalState.isDesktopWidget
        root: root
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
