/**
 * @brief 主函数
 *
 * 该函数是 MyTodo 应用程序的入口点。它初始化应用程序、设置日志系统、
 * 加载 QML 界面、处理命令行参数、设置应用程序信息和图标，最后启动应用程序事件循环。
 *
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return int 应用程序退出状态码
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-08-31 22:44:07(UTC+8) 周日
 * @version 0.4.0
 */
// Qt 相关头文件
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QSurfaceFormat>
#include <QtQuickControls2/QQuickStyle>
// Windows 相关头文件
#ifdef Q_OS_WIN
#include <windows.h>
#endif
// 自定义头文件
#include "cpp/foundation/logger.h"
#include "cpp/global_state.h"
#include "cpp/setting.h"
#include "cpp/todo/category_manager.h"
#include "cpp/todo/todo_filter.h"
#include "cpp/todo/todo_manager.h"
#include "cpp/todo/todo_sorter.h"
#include "cpp/user_auth.h"

#include <QDebug>
#include <QDirIterator>
void printResources(const QString &path = ":/") {
    QDirIterator it(path, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
}

int main(int argc, char *argv[]) {
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
    // Windows平台
    UINT cp = GetACP();
    SetConsoleCP(cp);
    SetConsoleOutputCP(cp);
#endif

    // printResources();

    QGuiApplication app(argc, argv);

    // 初始化日志系统
    qInstallMessageHandler(Logger::messageHandler);

    // 记录应用启动
    qInfo() << "MyTodo 应用程序启动";

    // 设置应用样式为Material
    qputenv("QT_QUICK_CONTROLS_STYLE", "Material");

    // 设置默认的 QQuickWindow 支持透明
    QSurfaceFormat fmt;
    fmt.setAlphaBufferSize(8); // 分配 alpha 通道
    QSurfaceFormat::setDefaultFormat(fmt);

    app.setWindowIcon(QIcon(":/image/icon.png")); // 设置窗口图标

    // 设置应用信息
    QGuiApplication::setApplicationName("MyTodo");
    QGuiApplication::setOrganizationName("MyTodo");
    QGuiApplication::setOrganizationDomain("mytodo.app");

    Setting &setting = Setting::GetInstance(); // 创建Setting实例

// 应用日志设置
#if defined(QT_DEBUG)
    setting.setLogLevel(Logger::LogLevel::Debug);
#else
    setting.setLogLevel(Logger::LogLevel::Info);
#endif
    setting.setLogToFile(setting.getLogToFile());
    setting.setLogToConsole(setting.getLogToConsole());
    setting.setMaxLogFileSize(setting.getMaxLogFileSize());
    setting.setMaxLogFiles(setting.getMaxLogFiles());

    qInfo() << "日志系统初始化完成，日志文件路径:" << setting.getLogFilePath();
    UserAuth &userAuth = UserAuth::GetInstance();     // 获取UserAuth实例
    TodoSyncServer todoSyncServer;                    // 创建TodoSyncServer实例
    CategoryManager categoryManager(&todoSyncServer); // 创建CategoryManager实例
    TodoManager todoManager;                          // 创建TodoManager实例
    GlobalState globalState;                          // 创建GlobalState实例

    // 检查是否通过开机自启动启动
    QStringList arguments = app.arguments();
    if (arguments.contains("--autostart")) {
        // 如果是开机自启动，默认设置为小组件模式
        globalState.setIsDesktopWidget(true);
    }

    QQmlApplicationEngine engine;

    // 将类注册到QML上下文
    engine.rootContext()->setContextProperty("setting", &setting);
    engine.rootContext()->setContextProperty("userAuth", &userAuth);
    engine.rootContext()->setContextProperty("todoSyncServer", &todoSyncServer);
    engine.rootContext()->setContextProperty("categoryManager", &categoryManager);
    engine.rootContext()->setContextProperty("todoFilter", todoManager.filter());
    engine.rootContext()->setContextProperty("todoSorter", todoManager.sorter());
    engine.rootContext()->setContextProperty("todoManager", &todoManager);
    engine.rootContext()->setContextProperty("globalState", &globalState);

    // 连接用户登录成功信号到TodoManager的更新用户UUID函数
    QObject::connect(&userAuth, &UserAuth::loginSuccessful, &todoManager,
                     [&todoManager]() { todoManager.updateAllTodosUserUuid(); });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("MyTodo", "Main");

    return app.exec();
}
