/**
 * @file qml_holiday.h
 * @brief QmlHolidayManager类的头文件
 *
 * 该文件定义了QmlHolidayManager类，作为HolidayManager的QML包装层。
 *
 * @author Sakurakugu
 * @date 2025-10-07 23:33:00(UTC+8) 周一
 */

#pragma once

#include "modules/holiday/holiday_manager.h"
#include <QDate>
#include <QObject>
#include <QVariantList>

/**
 * @class QmlHolidayManager
 * @brief 节假日管理器的QML包装类
 *
 * QmlHolidayManager类为QML提供节假日管理功能的接口：
 *
 * **核心功能：**
 * - 暴露HolidayManager的功能到QML
 * - 提供QML友好的数据类型转换
 * - 处理QML和C++之间的信号传递
 *
 * **QML接口：**
 * - 日期类型查询（工作日、节假日、周末）
 * - 节假日名称获取
 * - 日期计算（下一个工作日等）
 * - 年度节假日数据获取
 *
 * **使用场景：**
 * - DateTimePicker显示节假日信息
 * - RecurrenceInterval计算重复日期
 * - 其他需要节假日信息的QML组件
 */
class QmlHolidayManager : public QObject {
    Q_OBJECT

  public:
    explicit QmlHolidayManager(QObject *parent = nullptr);

    Q_INVOKABLE int getDateType(const QDate &date) const;        // 获取指定日期的类型
    Q_INVOKABLE bool isHoliday(const QDate &date) const;         // 检查是否为节假日（放假日）
    Q_INVOKABLE bool isWorkDay(const QDate &date) const;         // 检查是否为工作日
    Q_INVOKABLE bool isWeekend(const QDate &date) const;         // 检查是否为周末
    Q_INVOKABLE QString getHolidayName(const QDate &date) const; // 获取指定日期的节假日名称
    Q_INVOKABLE void refreshHolidayData(int year);               // 手动刷新指定年份的节假日数据

    Q_INVOKABLE QDate getNextWorkDay(const QDate &fromDate, int daysToAdd = 1) const; // 获取下一个工作日
    Q_INVOKABLE QDate getNextHoliday(const QDate &fromDate, int daysToAdd = 1) const; // 获取下一个节假日
    Q_INVOKABLE QDate getNextWeekend(const QDate &fromDate, int daysToAdd = 1) const; // 获取下一个周末

  private:
    HolidayManager &m_holidayManager;                                      ///< 节假日管理器引用
    QVariantMap holidayItemToVariantMap(const HolidayItem &holiday) const; // 将HolidayItem转换为QVariantMap
};