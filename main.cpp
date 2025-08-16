// Qt 相关头文件
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QSurfaceFormat>
#include <QtQuickControls2/QQuickStyle>
// Windows 相关头文件·
#ifdef Q_OS_WIN
#include <windows.h>
#endif
// 标准库头文件
#include <iostream>
// 自定义头文件
#include "cpp/mainWindow.h"
#include "cpp/settings.h"
#include "cpp/todoModel.h"

int main(int argc, char *argv[]) {
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
    // Windows平台
    UINT cp = GetACP();
    // UINT inputCP = GetConsoleCP();
    // UINT outputCP = GetConsoleOutputCP();
    // std::cout << "控制台输入代码页: " << inputCP << std::endl;
    // std::cout << "控制台输出代码页: " << outputCP << std::endl;

    if (cp == 65001) {
        // 设置 Windows 控制台输入输出为 UTF-8
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    } else if (cp == 936) {
        // 设置 Windows 控制台输入输出为 GBK
        SetConsoleCP(936);
        SetConsoleOutputCP(936);
    }

#endif

    QGuiApplication app(argc, argv);

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

    Settings settings;                       // 创建Settings实例
    TodoModel todoModel(nullptr, &settings); // 创建TodoModel实例
    MainWindow mainWindow;                   // 创建MainWindow实例
    
    // 检查是否通过开机自启动启动
    QStringList arguments = app.arguments();
    if (arguments.contains("--autostart")) {
        // 如果是开机自启动，默认设置为小组件模式
        mainWindow.setIsDesktopWidget(true);
    }

    QQmlApplicationEngine engine;

    // 将类注册到QML上下文
    engine.rootContext()->setContextProperty("todoModel", &todoModel);
    engine.rootContext()->setContextProperty("settings", &settings);
    engine.rootContext()->setContextProperty("mainWindow", &mainWindow);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("MyTodo", "Main");

    return app.exec();
}
