/**
 * @file global_state.h
 * @brief GlobalState类的头文件
 *
 * 该文件定义了GlobalState类，用于管理应用程序的全局状态。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @version 2025-08-22 16:24:28(UTC+8) 周五
 */
#pragma once

#include <QObject>

class GlobalState : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isDarkMode READ isDarkMode WRITE setIsDarkMode NOTIFY isDarkModeChanged)
    Q_PROPERTY(bool isDesktopWidget READ isDesktopWidget WRITE setIsDesktopWidget NOTIFY isDesktopWidgetChanged)
    Q_PROPERTY(bool isShowAddTask READ isShowAddTask WRITE setIsShowAddTask NOTIFY isShowAddTaskChanged)
    Q_PROPERTY(bool isShowTodos READ isShowTodos WRITE setIsShowTodos NOTIFY isShowTodosChanged)
    Q_PROPERTY(bool isShowSetting READ isShowSetting WRITE setIsShowSetting NOTIFY isShowSettingChanged)
    Q_PROPERTY(bool isShowDropdown READ isShowDropdown WRITE setIsShowDropdown NOTIFY isShowDropdownChanged)
    Q_PROPERTY(bool isSystemDarkMode READ isSystemDarkMode NOTIFY systemDarkModeChanged)
    Q_PROPERTY(bool preventDragging READ preventDragging WRITE setPreventDragging NOTIFY preventDraggingChanged)

  public:
    explicit GlobalState(QObject *parent = nullptr);

    // 属性访问器
    bool isDarkMode() const;
    void setIsDarkMode(bool value);

    bool isDesktopWidget() const;
    void setIsDesktopWidget(bool value);

    bool isShowAddTask() const;
    void setIsShowAddTask(bool value);

    bool isShowTodos() const;
    void setIsShowTodos(bool value);

    bool isShowSetting() const;
    void setIsShowSetting(bool value);

    bool isShowDropdown() const;
    void setIsShowDropdown(bool value);

    bool preventDragging() const;
    void setPreventDragging(bool value);

    bool isSystemDarkMode() const;

  public slots:
    // 从QML移动过来的函数
    void toggleWidgetMode();
    void toggleAddTaskVisible();  // 小工具模式--切换添加任务弹窗可见性
    void toggleTodosVisible();    // 小工具模式--切换待办列表弹窗可见性
    void toggleSettingsVisible(); // 小工具模式--切换设置弹窗可见性
    void toggleDropdownVisible(); // 小工具模式--切换下拉菜单可见性

    // 开机自启动相关方法
    Q_INVOKABLE bool isAutoStartEnabled() const;
    Q_INVOKABLE bool setAutoStart(bool enabled);

  private:
    void updateWidgetHeight(); // 动态更新小组件模式高度

  signals:
    void isDarkModeChanged();
    void isDesktopWidgetChanged();
    void isShowAddTaskChanged();
    void isShowTodosChanged();
    void isShowSettingChanged();
    void isShowDropdownChanged();
    void preventDraggingChanged();
    void systemDarkModeChanged();
    void widthChanged(int width);
    void heightChanged(int height);

  private:
    bool m_isDarkMode;
    bool m_isDesktopWidget;
    bool m_isShowAddTask;
    bool m_isShowTodos;
    bool m_isShowSetting;
    bool m_isShowDropdown;
    bool m_preventDragging;
};
