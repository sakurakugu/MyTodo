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
#include <QTimer>
#include <QtQuickControls2/QQuickStyle>
// Windows 相关头文件
#ifdef Q_OS_WIN
#include <windows.h>
#endif
// 自定义头文件
#include "cpp/foundation/database.h"
#include "cpp/foundation/logger.h"
#include "cpp/foundation/network_request.h"
#include "cpp/global_state.h"
#include "cpp/setting.h"
#include "cpp/todos/category/category_data_storage.h"
#include "cpp/todos/category/category_manager.h"
#include "cpp/todos/category/category_sync_server.h"
#include "cpp/todos/todo/todo_filter.h"
#include "cpp/todos/todo/todo_manager.h"
#include "cpp/todos/todo/todo_sorter.h"
#include "cpp/user_auth.h"
#include "version.h"

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
    // Windows平台，设置控制台编码
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    // printResources();

    QGuiApplication app(argc, argv);

    // 初始化日志系统
    qInstallMessageHandler(Logger::messageHandler);
// 应用日志设置
#if defined(QT_DEBUG)
    Logger::GetInstance().setLogLevel(LogLevel::Debug);
#else
    Logger::GetInstance().setLogLevel(LogLevel::Info);
#endif

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
    QGuiApplication::setApplicationName("MyTodo"); // 设置应用名称（不设置组织名）
    QGuiApplication::setApplicationVersion(APP_VERSION_STRING); // 设置应用版本

    GlobalState &globalState = GlobalState::GetInstance(); // 创建GlobalState实例
    Setting &setting = Setting::GetInstance();             // 创建Setting实例

    UserAuth userAuth;                                  // 获取UserAuth实例
    CategoryManager categoryManager(userAuth);          // 创建CategoryManager实例
    TodoManager todoManager(userAuth, categoryManager); // 创建TodoManager实例

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
    engine.rootContext()->setContextProperty("categoryManager", &categoryManager);
    engine.rootContext()->setContextProperty("todoManager", &todoManager);
    engine.rootContext()->setContextProperty("todoFilter", todoManager.filter());
    engine.rootContext()->setContextProperty("todoSorter", todoManager.sorter());
    engine.rootContext()->setContextProperty("todoSyncServer", todoManager.syncServer());
    engine.rootContext()->setContextProperty("globalState", &globalState);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // 应用退出前保存数据并关闭数据库，避免析构阶段访问 QSqlDatabase
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&]() {
        qInfo() << "应用即将退出，开始保存数据并关闭数据库";
        // 先让 Manager 显式保存
        todoManager.saveTodosToLocalStorage();

        // 给数据库操作一些时间完成
        QTimer::singleShot(100, [&]() {
            // 关闭数据库连接
            Database::GetInstance().closeDatabase();
            qInfo() << "保存与关闭完成";
        });
    });
    engine.loadFromModule("MyTodo", "Main");

    return app.exec();
}
