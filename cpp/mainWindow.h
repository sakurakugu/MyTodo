/**
 * @file mainWindow.h
 * @brief MainWindow类的头文件
 *
 * 该文件定义了MainWindow类，用于管理应用程序的主窗口。
 *
 * @author Sakurakugu
 * @date 2025
 */
#pragma once

#include <QObject>

class MainWindow : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isDesktopWidget READ isDesktopWidget WRITE setIsDesktopWidget NOTIFY isDesktopWidgetChanged)
    Q_PROPERTY(bool isShowAddTask READ isShowAddTask WRITE setIsShowAddTask NOTIFY isShowAddTaskChanged)
    Q_PROPERTY(bool isShowTodos READ isShowTodos WRITE setIsShowTodos NOTIFY isShowTodosChanged)
    Q_PROPERTY(bool isShowSetting READ isShowSetting WRITE setIsShowSetting NOTIFY isShowSettingChanged)

  public:
    explicit MainWindow(QObject *parent = nullptr);

    // 属性访问器
    bool isDesktopWidget() const;
    void setIsDesktopWidget(bool value);

    bool isShowAddTask() const;
    void setIsShowAddTask(bool value);

    bool isShowTodos() const;
    void setIsShowTodos(bool value);

    bool isShowSetting() const;
    void setIsShowSetting(bool value);

  public slots:
    // 从QML移动过来的函数
    void toggleWidgetMode();
    void toggleAddTaskVisible();
    void toggleTodosVisible();
    void toggleSettingsVisible();
    
    // 开机自启动相关方法
    Q_INVOKABLE bool isAutoStartEnabled() const;
    Q_INVOKABLE bool setAutoStart(bool enabled);

  private:
    void updateWidgetHeight(); // 动态更新小组件模式高度

  signals:
    void isDesktopWidgetChanged();
    void isShowAddTaskChanged();
    void isShowTodosChanged();
    void isShowSettingChanged();
    void widthChanged(int width);
    void heightChanged(int height);

  private:
    bool m_isDesktopWidget;
    bool m_isShowAddTask;
    bool m_isShowTodos;
    bool m_isShowSetting;
};
