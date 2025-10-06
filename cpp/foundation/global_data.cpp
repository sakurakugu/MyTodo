/**
 * @file global_data.cpp
 * @brief GlobalData类的实现文件
 *
 * 该文件实现了GlobalData类，负责管理应用程序的全局状态，包括窗口属性、显示状态和系统主题监听。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-10-05 23:50:47(UTC+8) 周日
 */
#include "global_data.h"
#include "config.h"

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QPalette>
#include <QSettings>

GlobalData::GlobalData() {}

GlobalData::~GlobalData() {}
