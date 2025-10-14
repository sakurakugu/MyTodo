/**
 * @file qml_holiday.cpp
 * @brief QmlHolidayManager类的实现
 *
 * 该文件实现了QmlHolidayManager类的所有方法。
 *
 * @author Sakurakugu
 * P25-10-07 23:33:00 (UTC+8) 周二
 */

#include "qml_holiday.h"
#include <QDebug>
#include <QVariantMap>

QmlHolidayManager::QmlHolidayManager(QObject *parent)
    : QObject(parent), m_holidayManager(HolidayManager::GetInstance()) {

    // 初始化节假日管理器
    m_holidayManager.初始化();
}

/**
 * @brief 获取指定日期的类型
 * @param date 日期
 * @return 日期类型（0=工作日，1=节假日，2=周末，3=节假日调休）
 */
int QmlHolidayManager::getDateType(const QDate &date) const {
    return static_cast<int>(m_holidayManager.获取日期类型(my::Date(date)));
}

/**
 * @brief 检查是否为节假日（放假日）
 * @param date 日期
 * @return 是否为节假日
 */
bool QmlHolidayManager::isHoliday(const QDate &date) const {
    return m_holidayManager.是否为节假日(my::Date(date));
}

/**
 * @brief 检查是否为工作日
 * @param date 日期
 * @return 是否为工作日
 */
bool QmlHolidayManager::isWorkDay(const QDate &date) const {
    return m_holidayManager.是否为工作日(my::Date(date));
}

/**
 * @brief 检查是否为周末
 * @param date 日期
 * @return 是否为周末
 */
bool QmlHolidayManager::isWeekend(const QDate &date) const {
    return m_holidayManager.是否为周末(my::Date(date));
}

/**
 * @brief 获取指定日期的节假日名称
 * @param date 日期
 * @return 节假日名称，如果不是节假日返回空字符串
 */
QString QmlHolidayManager::getHolidayName(const QDate &date) const {
    return QString::fromStdString(m_holidayManager.获取节假日名称(my::Date(date)));
}

/**
 * @brief 手动刷新指定年份的节假日数据
 * @param year 年份
 */
void QmlHolidayManager::refreshHolidayData(int year) {
    m_holidayManager.手动刷新指定年份的节假日数据(year);
}

/**
 * @brief 获取下一个工作日
 * @param fromDate 起始日期
 * @param daysToAdd 要添加的工作日数量
 * @return 下一个工作日
 */
QDate QmlHolidayManager::getNextWorkDay(const QDate &fromDate, int daysToAdd) const {
    return m_holidayManager.获取下一个工作日(my::Date(fromDate), daysToAdd).toQDate();
}

/**
 * @brief 获取下一个节假日
 * @param fromDate 起始日期
 * @param daysToAdd 要添加的节假日数量
 * @return 下一个节假日
 */
QDate QmlHolidayManager::getNextHoliday(const QDate &fromDate, int daysToAdd) const {
    return m_holidayManager.获取下一个节假日(my::Date(fromDate), daysToAdd).toQDate();
}

/**
 * @brief 获取下一个周末
 * @param fromDate 起始日期
 * @param daysToAdd 要添加的周末数量
 * @return 下一个周末
 */
QDate QmlHolidayManager::getNextWeekend(const QDate &fromDate, int daysToAdd) const {
    return m_holidayManager.获取下一个周末(my::Date(fromDate), daysToAdd).toQDate();
}

/**
 * @brief 将HolidayItem转换为QVariantMap
 * @param holiday 节假日项
 * @return QVariantMap
 */
QVariantMap QmlHolidayManager::holidayItemToVariantMap(const HolidayItem &holiday) const {
    QVariantMap map;
    map["date"] = holiday.date().toQDate();
    map["name"] = QString::fromStdString(holiday.name());
    map["isOffDay"] = holiday.isOffDay();
    return map;
}