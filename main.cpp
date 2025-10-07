/**
 * @file main.cpp
 * @brief 应用程序入口，初始化核心单例、日志、QML 引擎并启动事件循环
 *
 * 主要职责：
 * - 初始化 Qt 应用与基础图形环境（透明窗口、控件样式）
 * - 安装日志处理器并在 Debug 模式调整日志级别
 * - 构建并注入核心业务对象（UserAuth / CategoryManager / TodoManager / GlobalData / Setting）到 QML 上下文
 * - 处理 --autostart 参数以设置桌面组件模式
 * - 加载 QML 主界面并进入事件循环
 *
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 应用程序退出码
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */
#include <QDirIterator>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSurfaceFormat>
#include <QtQuickControls2/QQuickStyle>

#include "app/qml_category.h"
#include "app/qml_global_data.h"
#include "app/qml_setting.h"
#include "app/qml_todo.h"
#include "app/qml_update_checker.h"
#include "app/qml_user_auth.h"
#include "logger.h"
#include "modules/user/user_auth.h"
#include "qt_debug.h"
#include "version.h"

int main(int argc, char *argv[]) {
#ifdef QT_DEBUG
    QtDebug::设置终端编码();
    QtDebug::打印资源路径();
#endif

    QGuiApplication app(argc, argv);

    // 初始化日志系统
    qInstallMessageHandler(Logger::messageHandler);
// 应用日志设置
#ifdef QT_DEBUG
    Logger::GetInstance().setLogLevel(LogLevel::Debug); // 默认 Info，调试模式下设为 Debug
#endif

    qInfo() << std::format("{}/v{}", APP_NAME, APP_VERSION_STRING) << "应用程序启动";

    // 设置应用样式为Material
    qputenv("QT_QUICK_CONTROLS_STYLE", "Material");

    // 设置默认的 QQuickWindow 支持透明
    QSurfaceFormat fmt;
    fmt.setAlphaBufferSize(8); // 分配 alpha 通道
    QSurfaceFormat::setDefaultFormat(fmt);

    app.setWindowIcon(QIcon(":/image/icon.png")); // 设置窗口图标

    // 设置应用信息
    QGuiApplication::setApplicationName(APP_NAME);              // 设置应用名称（不设置组织名）
    QGuiApplication::setApplicationVersion(APP_VERSION_STRING); // 设置应用版本

    UserAuth userAuth;                               // 核心认证逻辑实例
    QmlUserAuth qmlUserAuth(userAuth);               // QML 认证层实例
    QmlCategoryManager qmlCategoryManager(userAuth); // QML 类别管理器实例
    QmlTodoManager qmlTodoManager(userAuth);         // QML 待办事项管理器实例
    QmlSetting qmlSetting;                           // QML 包装层实例
    QmlUpdateChecker qmlUpdateChecker;               // QML 更新检查器实例

    // 检查是否通过开机自启动启动
    QStringList arguments = app.arguments();
    if (arguments.contains("--autostart")) {
        // 如果是开机自启动，默认设置为小组件模式
        QmlGlobalData::GetInstance().setIsDesktopWidget(true);
    }

    QQmlApplicationEngine engine;

    // 将类注册到QML上下文
    engine.rootContext()->setContextProperty("globalData", &QmlGlobalData::GetInstance());
    engine.rootContext()->setContextProperty("setting", &qmlSetting);
    engine.rootContext()->setContextProperty("userAuth", &qmlUserAuth);
    engine.rootContext()->setContextProperty("categoryManager", &qmlCategoryManager);
    engine.rootContext()->setContextProperty("todoManager", &qmlTodoManager);
    engine.rootContext()->setContextProperty("updateChecker", &qmlUpdateChecker);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("MyTodo", "Main");

    return app.exec();
}
