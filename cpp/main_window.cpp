/**
 * @brief 主窗口类
 *
 * 该类负责管理应用程序的主窗口，包括窗口属性、显示状态和系统主题监听。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @version 2025-08-22 18:34:35(UTC+8) 周五
 */
#include "main_window.h"
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>

MainWindow::MainWindow(QObject *parent)
    : QObject(parent), m_isDesktopWidget(false), m_isShowAddTask(false), m_isShowTodos(true), m_isShowSetting(false) {

    // 监听系统主题变化
    connect(QGuiApplication::instance(), SIGNAL(paletteChanged(QPalette)), this, SIGNAL(systemDarkModeChanged()));
}

bool MainWindow::isDesktopWidget() const {
    return m_isDesktopWidget;
}

void MainWindow::setIsDesktopWidget(bool value) {
    if (m_isDesktopWidget != value) {
        m_isDesktopWidget = value;
        emit isDesktopWidgetChanged();
    }
}

bool MainWindow::isShowAddTask() const {
    return m_isShowAddTask;
}

void MainWindow::setIsShowAddTask(bool value) {
    if (m_isShowAddTask != value) {
        m_isShowAddTask = value;
        emit isShowAddTaskChanged();
    }
}

bool MainWindow::isShowTodos() const {
    return m_isShowTodos;
}

void MainWindow::setIsShowTodos(bool value) {
    if (m_isShowTodos != value) {
        m_isShowTodos = value;
        emit isShowTodosChanged();
    }
}

bool MainWindow::isShowSetting() const {
    return m_isShowSetting;
}

void MainWindow::setIsShowSetting(bool value) {
    if (m_isShowSetting != value) {
        m_isShowSetting = value;
        emit isShowSettingChanged();
    }
}

bool MainWindow::isSystemDarkMode() const {

#if defined(Q_OS_WIN) // Windows 平台
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                       QSettings::NativeFormat);
    int value = settings.value("AppsUseLightTheme", 1).toInt();
    return value == 0; // 0 = 深色，1 = 浅色

#elif defined(Q_OS_MACOS) // macOS 平台
    QProcess process;
    process.start("defaults", {"read", "-g", "AppleInterfaceStyle"});
    process.waitForFinished();
    QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    return (output.compare("Dark", Qt::CaseInsensitive) == 0);

#elif defined(Q_OS_LINUX) // Linux (GNOME / KDE)
    // 1. 先尝试 GNOME
    QProcess process;
    process.start("gsettings", {"get", "org.gnome.desktop.interface", "color-scheme"});
    process.waitForFinished();
    QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    if (output.contains("dark", Qt::CaseInsensitive))
        return true;

    // 2. KDE Plasma (5.24+ 支持 colorscheme)
    process.start("plasma-apply-colorscheme", {"--list-schemes"});
    process.waitForFinished();
    QString kdeOutput = QString::fromUtf8(process.readAllStandardOutput());
    if (kdeOutput.contains("Dark", Qt::CaseInsensitive))
        return true;

    return false; // 默认浅色

#else
    QPalette palette = QGuiApplication::palette();
    QColor windowColor = palette.color(QPalette::Window);
    QColor textColor = palette.color(QPalette::WindowText);

    // 判断系统是否为深色模式：窗口背景色比文本色更暗
    return windowColor.lightness() < textColor.lightness();
#endif
}

void MainWindow::toggleWidgetMode() {
    setIsDesktopWidget(!m_isDesktopWidget);

    // 根据模式设置窗口大小
    if (m_isDesktopWidget) {
        emit widthChanged(400);
        updateWidgetHeight(); // 动态计算高度
    } else {
        emit widthChanged(640);
        emit heightChanged(480);
    }
}

void MainWindow::updateWidgetHeight() {
    if (!m_isDesktopWidget)
        return;

    int totalHeight = 50; // 标题栏高度
    int popupSpacing = 6; // 弹窗间距

    // 计算需要显示的弹窗总高度
    if (m_isShowSetting) {
        totalHeight += 250 + popupSpacing;
    }
    if (m_isShowAddTask) {
        totalHeight += 250 + popupSpacing;
    }
    if (m_isShowTodos) {
        totalHeight += 200 + popupSpacing;
    }

    // 设置最小高度和额外边距
    int minHeight = 100;
    int extraMargin = 60;
    int finalHeight = qMax(minHeight, totalHeight + extraMargin);

    emit heightChanged(finalHeight);
}

void MainWindow::toggleAddTaskVisible() {
    setIsShowAddTask(!m_isShowAddTask);
    updateWidgetHeight(); // 动态更新高度
}

void MainWindow::toggleTodosVisible() {
    setIsShowTodos(!m_isShowTodos);
    updateWidgetHeight(); // 动态更新高度
}

void MainWindow::toggleSettingsVisible() {
    setIsShowSetting(!m_isShowSetting);
    updateWidgetHeight(); // 动态更新高度
}

bool MainWindow::isAutoStartEnabled() const {
    QSettings config("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    return config.contains("MyTodo");
}

bool MainWindow::setAutoStart(bool enabled) {
    QSettings config("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

    if (enabled) {
        QString applicationPath = QCoreApplication::applicationFilePath();
        applicationPath = QDir::toNativeSeparators(applicationPath);
        // 添加命令行参数标识开机自启动
        applicationPath += " --autostart";
        config.setValue("MyTodo", applicationPath);
    } else {
        config.remove("MyTodo");
    }

    config.sync();
    return config.status() == QSettings::NoError;
}