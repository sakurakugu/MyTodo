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
 * @author MyTodo Team
 * @date 2024
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
    // Qt.WindowStaysOnTopHint 只在 Debug 模式下启用
    flags: Qt.FramelessWindowHint | (isDesktopWidget ? Qt.Tool : Qt.Window) | Qt.WindowStaysOnTopHint

    // 窗口调整相关属性
    property int resizeBorderWidth: 5     ///< 边框调整大小的边距宽度

    // 显示模式控制属性
    property bool isDesktopWidget: mainWindow.isDesktopWidget  ///< 是否为桌面小组件模式
    property bool isShowTodos: mainWindow.isShowTodos         ///< 是否显示所有任务（小组件模式下）
    property bool isShowAddTask: mainWindow.isShowAddTask      ///< 是否显示添加任务界面
    property bool isShowSetting: mainWindow.isShowSetting      ///< 是否显示设置界面

    // 主题相关属性
    property bool isDarkMode: settings.get("setting/isDarkMode", false)  ///< 深色模式开关，从配置文件读取

    // 交互控制属性
    property bool preventDragging: settings.get("setting/preventDragging", false) ///< 是否禁止窗口拖拽，从配置文件读取

    // 页面导航系统（使用StackView实现）

    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: root.isDarkMode
    }

    /**
     * @brief 窗口尺寸同步连接
     *
     * 监听C++端mainWindow对象的尺寸变化，
     * 确保QML窗口与C++窗口尺寸保持同步。
     */
    Connections {
        target: mainWindow

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
        width: isDesktopWidget ? 400 : parent.width     ///< 小组件模式固定宽度，普通模式填充父容器
        height: isDesktopWidget ? 35 : 45               ///< 小组件模式较小高度，普通模式较大高度
        color: theme.primaryColor                       ///< 使用主题主色调
        border.color: theme.borderColor                 ///< 小组件模式显示边框
        border.width: isDesktopWidget ? 1 : 0           ///< 边框宽度
        radius: isDesktopWidget ? 5 : 0                 ///< 小组件模式圆角

        /**
         * @brief 窗口拖拽处理区域
         *
         * 处理标题栏的鼠标拖拽事件，实现窗口移动功能。
         * 支持防拖拽设置和不同模式下的拖拽控制。
         */
        MouseArea {
            anchors.fill: parent
            property point clickPos: "0,0"  ///< 记录鼠标按下时的位置

            /// 鼠标按下事件处理
            onPressed: function (mouse) {
                clickPos = Qt.point(mouse.x, mouse.y);
            }

            /// 鼠标移动事件处理 - 实现窗口拖拽
            onPositionChanged: function (mouse) {
                // 只有在非小组件模式或小组件模式但未启用防止拖动时才允许拖动
                if (pressed && ((!isDesktopWidget) || (isDesktopWidget && !preventDragging))) {
                    var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y);
                    root.x += delta.x;
                    root.y += delta.y;
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
            anchors.leftMargin: isDesktopWidget ? 5 : 10   ///< 左边距
            anchors.rightMargin: isDesktopWidget ? 5 : 10  ///< 右边距

            /**
             * @brief 页面导航区域
             *
             * 当在子页面时显示返回按钮和页面标题
             */
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10
                visible: !isDesktopWidget && stackView.depth > 1  ///< 仅在非小组件模式且有子页面时显示

                ToolButton {
                    text: "<-"
                    font.pixelSize: 16
                    onClicked: stackView.pop()
                }

                Label {
                    text: stackView.currentItem && stackView.currentItem.objectName === "settingPage" ? qsTr("设置") : ""
                    font.bold: true
                    font.pixelSize: 16
                    color: theme.textColor
                    Layout.leftMargin: 10
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
                spacing: 10
                visible: !isDesktopWidget && stackView.depth <= 1  ///< 仅在非小组件模式且在主页面时显示

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
                        var pos = mapToItem(null, 0, height);
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
                            radius: 15                          ///< 圆形头像
                            color: theme.secondaryBackgroundColor     ///< 使用主题次要背景色
                            Layout.alignment: Qt.AlignVCenter  ///< 垂直居中对齐

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
                            text: todoModel.username !== "" ? todoModel.username : "未登录"
                            color: theme.textColor                    ///< 使用主题文本颜色
                            font.bold: true                     ///< 粗体字
                            font.pixelSize: 14                  ///< 字体大小
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

            /**
             * @brief 小组件模式控制按钮组
             *
             * 仅在桌面小组件模式下显示的功能按钮，
             * 包括设置、任务列表展开/收起、添加任务等功能。
             */
            RowLayout {
                Layout.fillWidth: isDesktopWidget ? true : false   ///< 小组件模式下填充宽度
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter  ///< 右对齐垂直居中
                spacing: 2                                         ///< 按钮间距
                visible: isDesktopWidget                           ///< 仅在小组件模式下显示

                /// 设置按钮
                CustomButton {
                    text: "☰"                                       ///< 汉堡菜单图标
                    onClicked: mainWindow.toggleSettingsVisible()  ///< 切换设置界面显示
                    fontSize: 16
                    isDarkMode: root.isDarkMode
                }

                /// 任务列表展开/收起按钮
                CustomButton {
                    text: isShowTodos ? "^" : "v"                   ///< 根据状态显示箭头
                    onClicked: mainWindow.toggleTodosVisible()     ///< 切换任务列表显示
                    fontSize: 16
                    isDarkMode: root.isDarkMode
                }

                /// 添加任务按钮
                CustomButton {
                    text: "+"                                       ///< 加号图标
                    onClicked: mainWindow.toggleAddTaskVisible()   ///< 切换添加任务界面显示
                    fontSize: 16
                    isDarkMode: root.isDarkMode
                }

                CustomButton {
                    text: isDesktopWidget ? "大" : "小"
                    onClicked: {
                        if (isDesktopWidget) {
                            mainWindow.toggleWidgetMode();
                            mainWindow.isShowTodos = true;
                        } else {
                            mainWindow.toggleWidgetMode();
                        }
                    }
                    fontSize: 14
                    isDarkMode: root.isDarkMode
                }
            }

            // 非小组件模式按钮组
            RowLayout {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                spacing: 5
                visible: !isDesktopWidget

                CustomButton {
                    text: isDesktopWidget ? "大" : "小"
                    onClicked: {
                        if (isDesktopWidget) {
                            mainWindow.toggleWidgetMode();
                            mainWindow.isShowTodos = true;
                        } else {
                            mainWindow.toggleWidgetMode();
                        }
                    }
                    fontSize: 14
                    isDarkMode: root.isDarkMode
                }

                // 最小化按钮
                CustomButton {
                    text: "−"
                    onClicked: root.showMinimized()
                    fontSize: 14
                    isDarkMode: root.isDarkMode
                }

                // 最大化/恢复按钮
                CustomButton {
                    text: root.visibility === Window.Maximized ? "❐" : "□"
                    onClicked: {
                        if (root.visibility === Window.Maximized) {
                            root.showNormal();
                        } else {
                            root.showMaximized();
                        }
                    }
                    fontSize: 14
                    isDarkMode: root.isDarkMode
                }

                // 关闭按钮
                CustomButton {
                    text: "✕"
                    onClicked: root.close()
                    fontSize: 14
                    isDarkMode: root.isDarkMode
                }
            }
        }
    }

    // 边框调整大小区域
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

    // 导航栈，使用内置 Page 进行页面管理
    StackView {
        id: stackView
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        z: 1000
        focus: true
        visible: !isDesktopWidget && depth > 0 // 小窗口模式时隐藏主页面
        initialItem: mainPageComponent
    }

    // 小组件模式组件
    WidgetMode {
        id: widgetModeComponent
        anchors.fill: parent
        isDesktopWidget: root.isDesktopWidget
        isShowSetting: root.isShowSetting
        isShowAddTask: root.isShowAddTask
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

    // 顶部用户菜单（从头像处点击弹出）
    Menu {
        id: topMenu
        width: 200
        height: implicitHeight
        z: 10000  // 确保菜单显示在最上层

        background: Rectangle {
            color: root.isDarkMode ? "#2d3436" : "white"
            border.color: root.isDarkMode ? "#34495e" : "#cccccc"
            border.width: 1
            radius: 4
        }

        MenuItem {
            text: todoModel.isLoggedIn ? qsTr("退出登录") : qsTr("登录")
            onTriggered: {
                if (todoModel.isLoggedIn) {
                    logoutConfirmDialog.open();
                } else {
                    loginDialog.open();
                }
            }
        }

        MenuItem {
            text: qsTr("设置")
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
                Label {
                    text: qsTr("深色模式")
                    color: theme.textColor
                }
                Switch {
                    id: darkModeSwitch
                    checked: root.isDarkMode

                    property bool isInitialized: false

                    Component.onCompleted: {
                        isInitialized = true;
                    }

                    onCheckedChanged: {
                        if (!isInitialized) {
                            return; // 避免初始化时触发
                        }
                        root.isDarkMode = checked;
                        console.log("顶部用户菜单-切换深色模式", checked);
                        settings.save("setting/isDarkMode", checked);
                    }
                }
            }
        }

        MenuItem {
            id: onlineToggleItem
            contentItem: RowLayout {
                spacing: 12
                Label {
                    text: qsTr("自动同步")
                    color: theme.textColor
                }
                Switch {
                    id: onlineSwitch
                    checked: settings.get("setting/autoSync", false)

                    property bool isInitialized: false

                    Component.onCompleted: {
                        isInitialized = true;
                    }

                    onCheckedChanged: {
                        if (!isInitialized) {
                            return; // 避免初始化时触发
                        }

                        if (checked && !todoModel.isLoggedIn) {
                            // 如果要开启自动同步但未登录，显示提示并重置开关
                            onlineSwitch.checked = false;
                            loginRequiredDialog.open();
                        } else {
                            settings.save("setting/autoSync", checked);
                        }
                    }
                }
            }
            // 阻止点击整行触发默认切换
            onTriggered: {}
        }
    }
    // 通用确认对话框组件
    Component {
        id: confirmDialogComponent

        Dialog {
            id: confirmDialog
            title: qsTr("确认")
            standardButtons: Dialog.NoButton
            modal: true

            property string message: ""
            property string yesButtonText: qsTr("确定")
            property string noButtonText: qsTr("取消")

            contentItem: ColumnLayout {
                spacing: 20

                Label {
                    text: confirmDialog.message
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 10

                    Button {
                        text: confirmDialog.noButtonText
                        onClicked: confirmDialog.reject()

                        background: Rectangle {
                            color: parent.pressed ? (root.isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (root.isDarkMode ? "#3c5a78" : "#e0e0e0") : (root.isDarkMode ? "#2c3e50" : "#f0f0f0")
                            border.color: root.isDarkMode ? "#34495e" : "#cccccc"
                            border.width: 1
                            radius: 4
                        }

                        contentItem: Text {
                            text: parent.text
                            color: root.isDarkMode ? "#ecf0f1" : "black"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        text: confirmDialog.yesButtonText
                        onClicked: confirmDialog.accept()

                        background: Rectangle {
                            color: parent.pressed ? (root.isDarkMode ? "#34495e" : "#d0d0d0") : parent.hovered ? (root.isDarkMode ? "#3c5a78" : "#e0e0e0") : (root.isDarkMode ? "#2c3e50" : "#f0f0f0")
                            border.color: root.isDarkMode ? "#34495e" : "#cccccc"
                            border.width: 1
                            radius: 4
                        }

                        contentItem: Text {
                            text: parent.text
                            color: root.isDarkMode ? "#ecf0f1" : "black"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }
    }

    Component {
        id: mainPageComponent
        MainPage {
            isDarkMode: root.isDarkMode
            isDesktopWidget: root.isDesktopWidget
            rootWindow: root
        }
    }

    // TodoModel信号连接
    Connections {
        target: todoModel

        function onLoginSuccessful(username) {
            loginDialog.isLoggingIn = false;
            loginDialog.close();
            loginDialog.username = "";
            loginDialog.password = "";
            loginSuccessDialog.message = qsTr("欢迎回来，%1！").arg(username);
            loginSuccessDialog.open();
        }

        function onLoginFailed(errorMessage) {
            loginDialog.isLoggingIn = false;
            loginFailedDialog.message = qsTr("登录失败：%1").arg(errorMessage);
            loginFailedDialog.open();
        }

        function onLogoutSuccessful() {
            logoutSuccessDialog.open();
        }
    }

    // 登录成功提示对话框
    Dialog {
        id: loginSuccessDialog
        title: qsTr("登录成功")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok

        property string message: ""

        Label {
            text: loginSuccessDialog.message
            color: theme.textColor
        }

        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 8
        }
    }

    // 登录失败提示对话框
    Dialog {
        id: loginFailedDialog
        title: qsTr("登录失败")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok

        property string message: ""

        Label {
            text: loginFailedDialog.message
            color: theme.textColor
            wrapMode: Text.WordWrap
        }

        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 8
        }
    }

    // 退出登录确认对话框
    Dialog {
        id: logoutConfirmDialog
        title: qsTr("确认退出")
        modal: true
        anchors.centerIn: parent
        width: 300
        height: 150
        standardButtons: Dialog.NoButton

        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 8
        }

        contentItem: ColumnLayout {
            spacing: 20

            Label {
                text: qsTr("确定要退出登录吗？")
                color: theme.textColor
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10

                Button {
                    text: qsTr("取消")
                    onClicked: logoutConfirmDialog.close()
                    background: Rectangle {
                        color: theme.secondaryBackgroundColor
                        border.color: theme.borderColor
                        border.width: 1
                        radius: 4
                    }

                    contentItem: Text {
                        text: parent.text
                        color: theme.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    text: qsTr("确定")
                    onClicked: {
                        todoModel.logout();
                        logoutConfirmDialog.close();
                    }
                    background: Rectangle {
                        color: theme.primaryColor
                        border.color: theme.borderColor
                        border.width: 1
                        radius: 4
                    }

                    contentItem: Text {
                        text: parent.text
                        color: theme.textColor  
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    // 退出登录成功提示对话框
    Dialog {
        id: logoutSuccessDialog
        title: qsTr("退出成功")
        modal: true
        anchors.centerIn: parent
        width: 250
        height: 120
        standardButtons: Dialog.Ok

        Label {
            text: qsTr("已成功退出登录")
            color: theme.textColor
        }

        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 8
        }
    }

    // 登录弹窗
    Dialog {
        id: loginDialog
        title: qsTr("用户登录")
        modal: true
        anchors.centerIn: parent
        width: 350
        height: 280
        standardButtons: Dialog.NoButton

        property alias username: usernameField.text
        property alias password: passwordField.text
        property bool isLoggingIn: false

        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 8
        }

        contentItem: ColumnLayout {
            spacing: 20
            anchors.margins: 20

            Label {
                text: qsTr("请输入您的登录信息")
                color: theme.textColor      
                font.pixelSize: 16
                Layout.alignment: Qt.AlignHCenter
            }

            ColumnLayout {
                spacing: 15
                Layout.fillWidth: true

                // 用户名输入
                ColumnLayout {
                    spacing: 5
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("用户名:")
                        color: theme.textColor
                    }

                    TextField {
                        id: usernameField
                        placeholderText: qsTr("请输入用户名")
                        Layout.fillWidth: true
                        color: theme.textColor
                        background: Rectangle {
                            color: theme.secondaryBackgroundColor
                            border.color: theme.borderColor
                            border.width: 1
                            radius: 4
                        }
                    }
                }

                // 密码输入
                ColumnLayout {
                    spacing: 5
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("密码:")
                        color: theme.textColor
                    }

                    TextField {
                        id: passwordField
                        placeholderText: qsTr("请输入密码")
                        echoMode: TextInput.Password
                        Layout.fillWidth: true
                        color: theme.textColor
                        background: Rectangle {
                            color: theme.secondaryBackgroundColor
                            border.color: theme.borderColor
                            border.width: 1
                            radius: 4
                        }
                    }
                }
            }

            // 按钮区域
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10

                Button {
                    text: qsTr("取消")
                    onClicked: {
                        loginDialog.close();
                        usernameField.text = "";
                        passwordField.text = "";
                    }
                    background: Rectangle {
                        color: theme.secondaryBackgroundColor
                        border.color: theme.borderColor
                        border.width: 1
                        radius: 4
                    }

                    contentItem: Text {
                        text: parent.text
                        color: theme.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    text: loginDialog.isLoggingIn ? qsTr("登录中...") : qsTr("登录")
                    enabled: usernameField.text.length > 0 && passwordField.text.length > 0 && !loginDialog.isLoggingIn
                    onClicked: {
                        loginDialog.isLoggingIn = true;
                        todoModel.login(usernameField.text, passwordField.text);
                    }
                    background: Rectangle {
                        color: parent.enabled ? theme.primaryColor : theme.borderColor
                        border.color: theme.borderColor
                        border.width: 1
                        radius: 4
                    }

                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "white" : theme.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    // 登录提示对话框
    Dialog {
        id: loginRequiredDialog
        title: qsTr("需要登录")
        modal: true
        anchors.centerIn: parent
        width: 300
        height: 150
        standardButtons: Dialog.Ok

        background: Rectangle {
            color: theme.backgroundColor
            border.color: theme.borderColor
            border.width: 1
            radius: 8
        }

        Label {
            text: qsTr("开启自动同步功能需要先登录账户。\n请先登录后再开启自动同步。")
            wrapMode: Text.WordWrap
            color: theme.textColor
            anchors.centerIn: parent
        }
    }
}
