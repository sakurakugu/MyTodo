/**
 * @file qml_global_data.h
 * @brief QmlGlobalData类的头文件
 *
 * 该文件定义了QmlGlobalData类，用于管理Qml中的全局数据（目前是变量和函数）。
 *
 * @author Sakurakugu
 * @date 2025-10-06 17:36:23(UTC+8) 周一
 * @change 2025-10-06 17:36:23(UTC+8) 周一
 */
#pragma once
#include <QObject>
#include <QVariant>

class Config;

class QmlGlobalData : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isDarkMode READ isDarkMode WRITE setIsDarkMode NOTIFY isDarkModeChanged)
    Q_PROPERTY(bool isFollowSystemDarkMode READ isFollowSystemDarkMode WRITE setIsFollowSystemDarkMode NOTIFY
                   isFollowSystemDarkModeChanged)
    Q_PROPERTY(bool isDesktopWidget READ isDesktopWidget WRITE setIsDesktopWidget NOTIFY isDesktopWidgetChanged)
    Q_PROPERTY(bool isNew READ isNew WRITE setIsNew NOTIFY isNewChanged)
    Q_PROPERTY(bool isShowAddTask READ isShowAddTask WRITE setIsShowAddTask NOTIFY isShowAddTaskChanged)
    Q_PROPERTY(bool isShowTodos READ isShowTodos WRITE setIsShowTodos NOTIFY isShowTodosChanged)
    Q_PROPERTY(bool isShowSetting READ isShowSetting WRITE setIsShowSetting NOTIFY isShowSettingChanged)
    Q_PROPERTY(bool isShowDropdown READ isShowDropdown WRITE setIsShowDropdown NOTIFY isShowDropdownChanged)
    Q_PROPERTY(bool isSystemInDarkMode READ isSystemInDarkMode NOTIFY systemInDarkModeChanged)
    Q_PROPERTY(bool preventDragging READ preventDragging WRITE setPreventDragging NOTIFY preventDraggingChanged)
    Q_PROPERTY(bool refreshing READ refreshing WRITE setRefreshing NOTIFY refreshingChanged)
    Q_PROPERTY(QVariant selectedTodo READ selectedTodo WRITE setSelectedTodo NOTIFY selectedTodoChanged)

  public:
    // 单例模式
    static QmlGlobalData &GetInstance() {
        static QmlGlobalData instance;
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    QmlGlobalData(const QmlGlobalData &) = delete;
    QmlGlobalData &operator=(const QmlGlobalData &) = delete;
    QmlGlobalData(QmlGlobalData &&) = delete;
    QmlGlobalData &operator=(QmlGlobalData &&) = delete;

    // 属性访问器
    bool isDarkMode() const;
    void setIsDarkMode(bool value);

    bool isFollowSystemDarkMode() const;
    void setIsFollowSystemDarkMode(bool value);

    bool isDesktopWidget() const;
    void setIsDesktopWidget(bool value);

    bool isNew() const;
    void setIsNew(bool value);

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

    bool refreshing() const;
    void setRefreshing(bool value);

    QVariant selectedTodo() const;
    void setSelectedTodo(const QVariant &value);

    bool isSystemInDarkMode() const;

    // 从QML移动过来的函数
    Q_INVOKABLE QString formatDateTime(const QVariant &dateTime) const; // 格式化日期时间

  public slots:
    // 从QML移动过来的函数
    void toggleWidgetMode();      // 切换小工具模式
    void toggleAddTaskVisible();  // 小工具模式--切换添加任务弹窗可见性
    void toggleTodosVisible();    // 小工具模式--切换待办列表弹窗可见性
    void toggleSettingsVisible(); // 小工具模式--切换设置弹窗可见性

    // 开机自启动相关方法
    bool isAutoStartEnabled() const;
    bool setAutoStart(bool enabled);

  signals:
    void isDarkModeChanged();
    void isFollowSystemDarkModeChanged();
    void isDesktopWidgetChanged();
    void isNewChanged();
    void isShowAddTaskChanged();
    void isShowTodosChanged();
    void isShowSettingChanged();
    void isShowDropdownChanged();
    void preventDraggingChanged();
    void refreshingChanged();
    void selectedTodoChanged();
    void systemInDarkModeChanged();
    void widthChanged(int width);
    void heightChanged(int height);

  private:
    explicit QmlGlobalData(QObject *parent = nullptr);
    ~QmlGlobalData();

    // 配置文件
    Config &m_config;

    // 只在qml使用的变量
    bool m_isDarkMode;             // 深色模式
    bool m_isFollowSystemDarkMode; // 跟随系统深色模式
    bool m_preventDragging;        // 防止窗口拖动
    bool m_isDesktopWidget;        // 小工具模式
    bool m_isNew;                  // 小工具模式--是否是新创建的
    bool m_isShowAddTask;          // 小工具模式--添加任务弹窗可见性
    bool m_isShowTodos;            // 小工具模式--待办列表弹窗可见性
    bool m_isShowSetting;          // 小工具模式--设置弹窗可见性
    bool m_isShowDropdown;         // 小工具模式--下拉菜单可见性
    bool m_refreshing;             // 刷新中
    QVariant m_selectedTodo;       // 当前选中的待办事项
};