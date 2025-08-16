#include "mainWindow.h"

MainWindow::MainWindow(QObject *parent)
    : QObject(parent), m_isDesktopWidget(false), m_isShowAddTask(false), m_isShowTodos(true), m_isShowSetting(false) {
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

void MainWindow::toggleWidgetMode() {
    setIsDesktopWidget(!m_isDesktopWidget);

    // 根据模式设置窗口大小
    if (m_isDesktopWidget) {
        emit widthChanged(400);
    } else {
        emit widthChanged(640);
        emit heightChanged(480);
    }
}

void MainWindow::toggleAddTaskVisible() {
    setIsShowAddTask(!m_isShowAddTask);
}

void MainWindow::toggleTodosVisible() {
    setIsShowTodos(!m_isShowTodos);
}

void MainWindow::toggleSettingsVisible() {
    setIsShowSetting(!m_isShowSetting);
}