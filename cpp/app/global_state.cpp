/**
 * @file global_state.cpp
 * @brief GlobalState类的实现文件
 *
 * 该文件实现了GlobalState类，负责管理应用程序的全局状态，包括窗口属性、显示状态和系统主题监听。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-10-05 23:50:47(UTC+8) 周日
 */
#include "global_state.h"
#include "config.h"

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QPalette>
#include <QSettings>

GlobalState::GlobalState(QObject *parent)
    : QObject(parent),                                                  // 初始化父对象
      m_config(Config::GetInstance()),                                  // 初始化配置文件
      m_isDarkMode(m_config.get("setting/isDarkMode", false).toBool()), // 初始化是否为深色模式为false
      m_isFollowSystemDarkMode(
          m_config.get("setting/followSystemTheme", false).toBool()),             // 初始化是否跟随系统深色模式为false
      m_preventDragging(m_config.get("setting/preventDragging", false).toBool()), // 初始化是否防止窗口拖动为false
      //
      m_isDesktopWidget(false),  // 初始化是否为桌面窗口为false
      m_isNew(false),            // 初始化是否为新创建的为false
      m_isShowAddTask(false),    // 初始化是否显示添加任务为false
      m_isShowTodos(true),       // 初始化是否显示待办事项为true
      m_isShowSetting(false),    // 初始化是否显示设置为false
      m_isShowDropdown(false),   // 初始化是否显示下拉菜单为false
      m_refreshing(false),       // 初始化是否刷新中为false
      m_selectedTodo(QVariant()) // 初始化选中的待办事项为null
{
    // 监听系统主题变化
    connect(QGuiApplication::instance(), SIGNAL(paletteChanged(QPalette)), this, SIGNAL(systemInDarkModeChanged()));
}

GlobalState::~GlobalState() {
    // 保存配置
    m_config.save("setting/isDarkMode", m_isDarkMode);
    m_config.save("setting/followSystemTheme", m_isFollowSystemDarkMode);
    m_config.save("setting/preventDragging", m_preventDragging);
}

bool GlobalState::isDarkMode() const {
    return m_isDarkMode;
}

void GlobalState::setIsDarkMode(bool value) {
    if (m_isDarkMode != value) {
        m_isDarkMode = value;
        m_config.save("setting/isDarkMode", value);
        emit isDarkModeChanged();
    }
}

bool GlobalState::isFollowSystemDarkMode() const {
    return m_isFollowSystemDarkMode;
}

void GlobalState::setIsFollowSystemDarkMode(bool value) {
    if (m_isFollowSystemDarkMode != value) {
        m_isFollowSystemDarkMode = value;
        m_config.save("setting/followSystemTheme", value);
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
        m_config.save("setting/preventDragging", value);
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

/**
 * @brief 格式化日期时间
 *
 * 该函数根据输入的日期时间对象，返回一个格式化后的字符串表示。
 * 格式化规则如下：
 * - 若时间在当前时间的1分钟内，返回"刚刚"
 * - 若时间在当前时间的1小时内，返回"X分钟前"
 * - 若时间在当前时间的24小时内，返回"X:XX"
 * - 若时间在当前时间的24小时外，且在同一年内，返回"MM/DD"
 * - 若时间跨年度，返回"YYYY/MM/DD"
 *
 * @param dt 输入的日期时间对象
 * @return 格式化后的字符串表示
 */
QString GlobalState::formatDateTime(const QVariant &dateTime) const {
    // 确保输入是有效的QDateTime
    QDateTime dt;
    if (!dateTime.canConvert<QDateTime>()) {
        return "";
    } else {
        dt = dateTime.toDateTime();
    }

    if (!dt.isValid()) {
        return "";
    }

    QDateTime now = QDateTime::currentDateTime();
    qint64 timeDiff = now.toMSecsSinceEpoch() - dt.toMSecsSinceEpoch();
    qint64 minutesDiff = timeDiff / (1000 * 60);
    qint64 hoursDiff = timeDiff / (1000 * 60 * 60);
    qint64 daysDiff = timeDiff / (1000 * 60 * 60 * 24);

    // 今天
    if (daysDiff == 0) {
        // 小于1分钟
        if (minutesDiff < 1) {
            return tr("刚刚");
        } else if (hoursDiff < 1) { // 小于1小时
            return tr("%1分钟前").arg(minutesDiff);
        } else { // 显示具体时间
            int hours = dt.time().hour();
            int minutes = dt.time().minute();
            return QString("%1:%2").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0'));
        }
    } else if (daysDiff == 1) { // 昨天
        return tr("昨天");
    } else if (daysDiff == 2) { // 前天
        return tr("前天");
    } else if (dt.date().year() == now.date().year()) { // 今年内
        int month = dt.date().month();
        int day = dt.date().day();
        return QString("%1/%2").arg(month, 2, 10, QChar('0')).arg(day, 2, 10, QChar('0'));
    } else { // 跨年
        int year = dt.date().year();
        int month = dt.date().month();
        int day = dt.date().day();
        return QString("%1/%2/%3")
            .arg(year, 4, 10, QChar('0'))
            .arg(month, 2, 10, QChar('0'))
            .arg(day, 2, 10, QChar('0'));
    }
}