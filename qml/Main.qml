/**
 * @file Main.qml
 * @brief 应用程序主窗口
 *
 * 该文件定义了MyTodo应用程序的主窗口界面，包括：
 * - 无边框窗口设计
 * - 桌面小组件模式支持
 * - 深色/浅色主题切换
 * - 窗口拖拽和调整大小功能
 * - 页面导航和状态管理
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @version 2025-08-23 21:09:00(UTC+8) 周六
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "components"

/**
 * @brief 应用程序主窗口
 *
 * 主窗口采用无边框设计，支持桌面小组件模式和普通窗口模式。
 * 提供了完整的主题系统和响应式布局。
 */
Window {
    id: root
    width: 640
    height: 480
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: qsTr("我的待办")

    // 背景透明度设置 - 必须保持透明，否则在Windows下会出现黑色背景问题
    color: "transparent"

    // 窗口标志设置 - FramelessWindowHint必须始终存在，否则Windows下会出现背景变黑且无法恢复的问题
    // TODO: Qt.WindowStaysOnTopHint 只在 Debug 模式下启用
    flags: Qt.FramelessWindowHint | (isDesktopWidget ? Qt.Tool : Qt.Window) | Qt.WindowStaysOnTopHint

    // 显示模式控制属性
    property bool isDesktopWidget: globalState.isDesktopWidget  ///< 是否为桌面小组件模式
    property bool isShowTodos: globalState.isShowTodos          ///< 是否显示所有任务（小组件模式下）
    property bool isDarkMode: setting.get("setting/isDarkMode", false)           ///< 深色模式开关，从配置文件读取
    property bool preventDragging: setting.get("setting/preventDragging", false) ///< 是否禁止窗口拖拽，从配置文件读取

    FontLoader {
        id: iconFont
        source: "qrc:/qt/qml/MyTodo/image/font_icon/iconfont.ttf"
    }

    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: root.isDarkMode
    }

    /**
     * @brief 主窗口背景容器
     *
     * 为整个窗口提供圆角背景效果，包裹所有内容。
     */
    Rectangle {
        id: mainBackground
        visible: !root.isDesktopWidget
        anchors.fill: parent
        anchors.margins: 0
        color: theme.backgroundColor
        radius: 5                        ///< 窗口圆角半径
        border.color: theme.borderColor
        border.width: 1
        z: -1000                        ///< 确保背景在所有内容下方
    }

    /**
     * @brief 窗口尺寸同步连接
     *
     * 监听C++端globalState对象的尺寸变化，
     * 确保QML窗口与C++窗口尺寸保持同步。
     * 仅在桌面小组件模式下有效。
     */
    Connections {
        target: globalState

        /// 处理窗口宽度变化
        function onWidthChanged(width) {
            root.width = width;
        }

        /// 处理窗口高度变化
        function onHeightChanged(height) {
            root.height = height;
        }
    }

    /**
     * @brief 应用程序标题栏
     *
     * 自定义标题栏，支持窗口拖拽功能。
     * 在桌面小组件模式和普通窗口模式下有不同的样式。
     */
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        width: root.isDesktopWidget ? 400 : parent.width     ///< 小组件模式固定宽度，普通模式填充父容器
        height: root.isDesktopWidget ? 35 : 48               ///< 小组件模式较小高度，普通模式较大高度
        color: theme.primaryColor                            ///< 使用主题主色调
        border.color: theme.borderColor                      ///< 小组件模式显示边框
        border.width: 0                                     ///< 边框宽度
        topLeftRadius: 5                                     ///< 左上角圆角
        topRightRadius: 5                                    ///< 右上角圆角
        bottomLeftRadius: root.isDesktopWidget ? 5 : 0       ///< 左下角无圆角
        bottomRightRadius: root.isDesktopWidget ? 5 : 0      ///< 右下角无圆角

        /**
         * @brief 窗口拖拽处理区域
         *
         * 处理标题栏的鼠标拖拽事件，实现窗口移动功能。
         * 支持防拖拽设置和不同模式下的拖拽控制。
         */
        MouseArea {
            anchors.fill: parent
            property point clickPos: Qt.point(0, 0)  ///< 记录鼠标按下时的位置
            property bool wasMaximized: false        ///< 记录拖拽开始时是否为最大化状态

            /// 鼠标按下事件处理
            onPressed: function (mouse) {
                clickPos = Qt.point(mouse.x, mouse.y);
                wasMaximized = (root.visibility === Window.Maximized);
            }

            /// 鼠标移动事件处理 - 实现窗口拖拽
            onPositionChanged: function (mouse) {
                // 只有在非小组件模式或小组件模式但未启用防止拖动时才允许拖动
                if (pressed && ((!root.isDesktopWidget) || (root.isDesktopWidget && !root.preventDragging))) {
                    if (wasMaximized) {
                        // 如果是从最大化状态开始拖拽，需要特殊处理
                        var mouseRatioX = clickPos.x / titleBar.width;  // 计算鼠标在标题栏中的相对位置
                        root.showNormal();
                        // 根据鼠标相对位置调整窗口位置，使鼠标保持在相同的相对位置
                        var globalMousePos = mapToGlobal(mouse.x, mouse.y);
                        root.x = globalMousePos.x - (root.width * mouseRatioX);
                        root.y = globalMousePos.y - mouse.y;
                        // 更新点击位置为新窗口大小下的相对位置
                        clickPos = Qt.point(root.width * mouseRatioX, mouse.y);
                        wasMaximized = false;
                    } else {
                        // 普通拖拽逻辑
                        var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y);
                        root.x += delta.x;
                        root.y += delta.y;
                    }
                }
            }
            z: -1 ///< 确保此MouseArea在其他控件下层，不影响其他控件的点击事件
        }

        /**
         * @brief 标题栏内容布局
         *
         * 包含用户信息区域和控制按钮组，
         * 根据不同模式显示不同的内容。
         */
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: root.isDesktopWidget ? 5 : 10   ///< 左边距
            anchors.rightMargin: root.isDesktopWidget ? 5 : 10  ///< 右边距

            /**
             * @brief 页面导航区域
             *
             * 当在子页面时显示返回按钮和页面标题
             */
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0
                visible: !root.isDesktopWidget && stackView.depth > 1  ///< 仅在非小组件模式且有子页面时显示

                ToolButton {
                    font.family: iconFont.name
                    text: "\ue8fa"
                    font.bold: true
                    font.pixelSize: 18
                    onClicked: stackView.pop()
                }

                Label {
                    text: stackView.currentItem && stackView.currentItem.objectName === "settingPage" ? qsTr("设置") : ""
                    font.bold: true                     ///< 粗体字
                    font.pixelSize: 16
                    color: theme.titleBarTextColor
                }
            }

            /**
             * @brief 用户信息显示区域
             *
             * 仅在普通窗口模式下显示，包含用户头像和用户名。
             * 点击可弹出用户菜单。
             */
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: !root.isDesktopWidget && stackView.depth <= 1  ///< 仅在非小组件模式且在主页面时显示

                /**
                 * @brief 用户资料点击区域
                 *
                 * 处理用户头像和信息的点击事件，弹出用户菜单。
                 */
                MouseArea {
                    id: userProfileMouseArea
                    Layout.preferredWidth: userProfileContent.width
                    Layout.preferredHeight: userProfileContent.height
                    Layout.alignment: Qt.AlignVCenter
                    z: 10  // 确保在拖拽MouseArea之上

                    /// 点击弹出用户菜单
                    onClicked: {
                        // 计算菜单位置，固定在用户头像下方
                        var pos = mapToItem(null, 0, height + 9);
                        topMenu.popup(pos.x, pos.y);
                    }

                    RowLayout {
                        id: userProfileContent
                        spacing: 10

                        /**
                         * @brief 用户头像显示
                         *
                         * 圆形头像容器，显示默认用户图标。
                         */
                        Rectangle {
                            width: 30
                            height: 30
                            radius: 15                               ///< 圆形头像
                            color: theme.secondaryBackgroundColor    ///< 使用主题次要背景色
                            Layout.alignment: Qt.AlignVCenter        ///< 垂直居中对齐

                            /// 头像图标
                            Text {
                                anchors.centerIn: parent
                                text: "👤"                      ///< 默认用户图标
                                font.pixelSize: 18
                            }
                        }

                        /**
                         * @brief 用户名显示
                         *
                         * 显示当前登录用户的用户名，未登录时显示提示文本。
                         */
                        Text {
                            text: userAuth.username !== "" ? userAuth.username : qsTr("未登录")
                            color: theme.titleBarTextColor      ///< 使用主题文本颜色
                            font.bold: true                     ///< 粗体字
                            font.pixelSize: 16                  ///< 字体大小
                            Layout.alignment: Qt.AlignVCenter   ///< 垂直居中对齐
                            horizontalAlignment: Text.AlignLeft ///< 水平左对齐
                        }
                    }
                }

                /// 标题栏剩余空间填充
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            /// 设置按钮
            IconButton {
                text: "\ue90f"                                       ///< 菜单图标
                onClicked: globalState.toggleSettingsVisible()  ///< 切换设置界面显示
                visible: root.isDesktopWidget
                textColor: theme.titleBarTextColor
                fontSize: 16
                isDarkMode: root.isDarkMode
            }

            /// 待办状态指示器
            Text {
                id: todoStatusIndicator
                visible: root.isDesktopWidget
                Layout.alignment: Qt.AlignVCenter
                color: theme.titleBarTextColor
                font.pixelSize: 14
                font.bold: true
                
                // TODO: 新增或删除待办后，这里的文字没有更改（显示的是当前分类下的待办数量）
                property int todoCount: todoManager.rowCount()
                property bool isHovered: false
                
                text: {
                    if (isHovered) {
                        return todoCount > 0 ? todoCount + "个待办" : "没有待办"
                    } else {
                        return todoCount > 0 ? todoCount + "个待办" : "我的待办"
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    
                    onEntered: {
                        parent.isHovered = true
                    }
                    
                    onExited: {
                        parent.isHovered = false
                    }
                }
            }

            /// 标题栏剩余空间填充
            Item {
                visible: root.isDesktopWidget
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            /// 任务列表展开/收起按钮
            IconButton {
                text: isShowTodos ? "\ue667" : "\ue669"        ///< 根据状态显示箭头
                onClicked: globalState.toggleTodosVisible()     ///< 切换任务列表显示
                visible: root.isDesktopWidget
                textColor: theme.titleBarTextColor
                fontSize: 16
                isDarkMode: root.isDarkMode
            }

            /// 添加任务按钮
            IconButton {
                text: "\ue903"                                 ///< 加号图标
                onClicked: globalState.toggleAddTaskVisible()   ///< 切换添加任务界面显示
                visible: root.isDesktopWidget
                textColor: theme.titleBarTextColor
                fontSize: 16
                isDarkMode: root.isDarkMode
            }

            /// 普通模式和小组件模式切换按钮
            IconButton {
                text: root.isDesktopWidget ? "\ue620" : "\ue61f"
                /// 鼠标按下事件处理
                onClicked: {
                    if (root.isDesktopWidget) {
                        globalState.toggleWidgetMode();
                        globalState.isShowTodos = true;
                    } else {
                        globalState.toggleWidgetMode();
                    }
                }
                textColor: theme.titleBarTextColor
                fontSize: 18
                isDarkMode: root.isDarkMode
            }

            // 最小化按钮
            IconButton {
                text: "\ue65a"
                onClicked: root.showMinimized()
                textColor: theme.titleBarTextColor
                visible: !root.isDesktopWidget
                fontSize: 16
                isDarkMode: root.isDarkMode
            }

            // 最大化/恢复按钮
            IconButton {
                text: root.visibility === Window.Maximized ? "\ue600" : "\ue65b"
                visible: !root.isDesktopWidget
                onClicked: {
                    if (root.visibility === Window.Maximized) {
                        root.showNormal();
                    } else {
                        root.showMaximized();
                    }
                }
                textColor: theme.titleBarTextColor
                fontSize: 16
                isDarkMode: root.isDarkMode
            }

            // 关闭按钮
            IconButton {
                text: "\ue8d1"
                onClicked: root.close()
                visible: !root.isDesktopWidget
                fontSize: 16
                textColor: theme.titleBarTextColor
                isDarkMode: root.isDarkMode
            }
        }
    }

    // 边框调整大小区域
    property int resizeBorderWidth: 5     ///< 边框调整大小的边距宽度
    // 左边框
    MouseArea {
        id: leftResizeArea
        width: resizeBorderWidth
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: !isDesktopWidget
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
        width: resizeBorderWidth
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: !isDesktopWidget
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
        height: resizeBorderWidth
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        visible: !isDesktopWidget
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
        height: resizeBorderWidth
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: !isDesktopWidget
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
        width: resizeBorderWidth
        height: resizeBorderWidth
        anchors.left: parent.left
        anchors.top: parent.top
        visible: !isDesktopWidget
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
        width: resizeBorderWidth
        height: resizeBorderWidth
        anchors.right: parent.right
        anchors.top: parent.top
        visible: !isDesktopWidget
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
        width: resizeBorderWidth
        height: resizeBorderWidth
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        visible: !isDesktopWidget
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
        width: resizeBorderWidth
        height: resizeBorderWidth
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: !isDesktopWidget
        cursorShape: Qt.SizeFDiagCursor
        z: 2000

        property string edge: "bottomright"

        onPressed: {
            root.startSystemResize(Qt.BottomEdge | Qt.RightEdge);
        }
    }

    // 导航栈，进行页面管理
    StackView {
        id: stackView
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 0
        z: 1000
        focus: true
        visible: !root.isDesktopWidget && depth > 0 // 小窗口模式时隐藏主页面
        initialItem: mainPageComponent
        clip: true  ///< 裁剪内容以配合窗口圆角效果
    }

    // 小组件模式组件
    WidgetMode {
        id: widgetModeComponent
        anchors.fill: parent
        isDesktopWidget: root.isDesktopWidget
        isShowTodos: root.isShowTodos
        isDarkMode: root.isDarkMode
        preventDragging: root.preventDragging
        visible: isDesktopWidget

        // 连接信号
        onPreventDraggingToggled: function (value) {
            root.preventDragging = value;
        }

        onDarkModeToggled: function (value) {
            root.isDarkMode = value;
        }
    }

    MainPage {
        id: mainPageComponent
        isDarkMode: root.isDarkMode
        isDesktopWidget: root.isDesktopWidget
        rootWindow: root
    }

    // 顶部用户菜单（从头像处点击弹出）
    Menu {
        id: topMenu
        width: 200
        height: implicitHeight
        z: 10000  // 确保菜单显示在最上层

        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 4
        }

        MenuItem {
            text: todoManager.isLoggedIn ? qsTr("退出登录") : qsTr("登录")
            contentItem: Row {
                spacing: 8
                Text {
                    text: "\ue981"
                    font.family: iconFont.name
                    color: theme.textColor
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: parent.parent.text
                    color: theme.textColor
                    // font: parent.parent.font
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            onTriggered: {
                if (todoManager.isLoggedIn) {
                    loginStatusDialogs.showLogoutConfirm();
                } else {
                    loginStatusDialogs.showLoginDialog();
                }
            }
        }

        MenuItem {
            text: qsTr("设置")
            contentItem: Row {
                spacing: 8
                Text {
                    text: "\ue913"
                    font.family: iconFont.name
                    color: theme.textColor
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: parent.parent.text
                    color: theme.textColor
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            onTriggered: {
                stackView.push(Qt.resolvedUrl("Setting.qml"), {
                    isDarkMode: root.isDarkMode,
                    preventDragging: root.preventDragging,
                    rootWindow: root
                });
            }
        }

        MenuItem {
            id: darkModeItem
            contentItem: RowLayout {
                spacing: 12
                Row {
                    spacing: 8
                    Text {
                        text: root.isDarkMode ? "\ue668" : "\ue62e"
                        font.family: iconFont.name
                        color: theme.textColor
                        font.pixelSize: 18
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: qsTr("深色模式")
                        color: theme.textColor
                        font.pixelSize: 18
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Switch {
                    id: darkModeSwitch
                    leftPadding: 0
                    checked: root.isDarkMode
                    scale: 0.7

                    onCheckedChanged: {
                        root.isDarkMode = checked;
                        setting.save("setting/isDarkMode", checked);
                    }
                }
            }
        }

        MenuItem {
            id: onlineToggleItem
            contentItem: RowLayout {
                spacing: 12
                Row {
                    spacing: 8
                    Text {
                        text: "\ue8ef"
                        font.family: iconFont.name
                        color: theme.textColor
                        font.pixelSize: 18
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: qsTr("自动同步")
                        color: theme.textColor
                        font.pixelSize: 18
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Switch {
                    id: onlineSwitch
                    leftPadding: 0
                    checked: setting.get("setting/autoSync", false)
                    scale: 0.7

                    onCheckedChanged: {
                        if (checked && !todoManager.isLoggedIn) {
                            // 如果要开启自动同步但未登录，显示提示并重置开关
                            onlineSwitch.checked = false;
                            topMenu.close(); // 关闭菜单
                            loginStatusDialogs.showLoginRequired();
                        } else {
                            setting.save("setting/autoSync", checked);
                        }
                    }
                }
            }
            // 阻止点击整行触发默认切换
            onTriggered: {}
        }
    }

    // 登录相关对话框组件
    LoginStatusDialogs {
        id: loginStatusDialogs
        isDarkMode: root.isDarkMode
    }
}
