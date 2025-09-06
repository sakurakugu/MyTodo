/**
 * @brief 全局状态类
 *
 * 该类负责管理应用程序的全局状态，包括窗口属性、显示状态和系统主题监听。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-06 16:55:02(UTC+8) 周六
 * @version 0.4.0
 */
#include "global_state.h"
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>

GlobalState::GlobalState(QObject *parent)
    : QObject(parent),                 // 初始化父对象
      m_isDarkMode(false),             // 初始化是否为深色模式为false
      m_isFollowSystemDarkMode(false), // 初始化是否跟随系统深色模式为false
      m_isDesktopWidget(false),        // 初始化是否为桌面窗口为false
      m_isNew(false),                  // 初始化是否为新创建的为false
      m_isShowAddTask(false),          // 初始化是否显示添加任务为false
      m_isShowTodos(true),             // 初始化是否显示待办事项为true
      m_isShowSetting(false),          // 初始化是否显示设置为false
      m_isShowDropdown(false),         // 初始化是否显示下拉菜单为false
      m_preventDragging(false),        // 初始化是否防止窗口拖动为false
      m_refreshing(false),             // 初始化是否刷新中为false
      m_selectedTodo(QVariant()) {     // 初始化选中的待办事项为null

    // 监听系统主题变化
    connect(QGuiApplication::instance(), SIGNAL(paletteChanged(QPalette)), this, SIGNAL(systemInDarkModeChanged()));
}

bool GlobalState::isDarkMode() const {
    return m_isDarkMode;
}

void GlobalState::setIsDarkMode(bool value) {
    if (m_isDarkMode != value) {
        m_isDarkMode = value;
        emit isDarkModeChanged();
    }
}

bool GlobalState::isFollowSystemDarkMode() const {
    return m_isFollowSystemDarkMode;
}

void GlobalState::setIsFollowSystemDarkMode(bool value) {
    if (m_isFollowSystemDarkMode != value) {
        m_isFollowSystemDarkMode = value;
        emit isFollowSystemDarkModeChanged();
    }
}

bool GlobalState::isDesktopWidget() const {
    return m_isDesktopWidget;
}

void GlobalState::setIsDesktopWidget(bool value) {
    if (m_isDesktopWidget != value) {
        m_isDesktopWidget = value;
        emit isDesktopWidgetChanged();
    }
}

bool GlobalState::isNew() const {
    return m_isNew;
}

void GlobalState::setIsNew(bool value) {
    if (m_isNew != value) {
        m_isNew = value;
        emit isNewChanged();
    }
}

bool GlobalState::isShowAddTask() const {
    return m_isShowAddTask;
}

void GlobalState::setIsShowAddTask(bool value) {
    if (m_isShowAddTask != value) {
        m_isShowAddTask = value;
        emit isShowAddTaskChanged();
    }
}

bool GlobalState::isShowTodos() const {
    return m_isShowTodos;
}

void GlobalState::setIsShowTodos(bool value) {
    if (m_isShowTodos != value) {
        m_isShowTodos = value;
        emit isShowTodosChanged();
    }
}

bool GlobalState::isShowSetting() const {
    return m_isShowSetting;
}

void GlobalState::setIsShowSetting(bool value) {
    if (m_isShowSetting != value) {
        m_isShowSetting = value;
        emit isShowSettingChanged();
    }
}

bool GlobalState::isShowDropdown() const {
    return m_isShowDropdown;
}

void GlobalState::setIsShowDropdown(bool value) {
    if (m_isShowDropdown != value) {
        m_isShowDropdown = value;
        emit isShowDropdownChanged();
    }
}

bool GlobalState::preventDragging() const {
    return m_preventDragging;
}

void GlobalState::setPreventDragging(bool value) {
    if (m_preventDragging != value) {
        m_preventDragging = value;
        emit preventDraggingChanged();
    }
}

bool GlobalState::refreshing() const {
    return m_refreshing;
}

void GlobalState::setRefreshing(bool value) {
    if (m_refreshing != value) {
        m_refreshing = value;
        emit refreshingChanged();
    }
}

QVariant GlobalState::selectedTodo() const {
    return m_selectedTodo;
}

void GlobalState::setSelectedTodo(const QVariant &value) {
    if (m_selectedTodo != value) {
        m_selectedTodo = value;
        emit selectedTodoChanged();
    }
}

bool GlobalState::isSystemInDarkMode() const {

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

void GlobalState::toggleWidgetMode() {
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

void GlobalState::updateWidgetHeight() {
    if (!m_isDesktopWidget)
        return;

    // TODO: 之后再说，到时候从WidgetMode中移动进来

    emit heightChanged(1);
}

void GlobalState::toggleAddTaskVisible() {
    setIsShowAddTask(!m_isShowAddTask);
    updateWidgetHeight(); // 动态更新高度
}

void GlobalState::toggleTodosVisible() {
    setIsShowTodos(!m_isShowTodos);
    updateWidgetHeight(); // 动态更新高度
}

void GlobalState::toggleSettingsVisible() {
    setIsShowSetting(!m_isShowSetting);
    updateWidgetHeight(); // 动态更新高度
}

void GlobalState::toggleDropdownVisible() {
    setIsShowDropdown(!m_isShowDropdown);
    updateWidgetHeight(); // 动态更新高度
}

bool GlobalState::isAutoStartEnabled() const {
    QSettings config("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    return config.contains("MyTodo");
}

bool GlobalState::setAutoStart(bool enabled) {
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