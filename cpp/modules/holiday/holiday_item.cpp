/**
 * @file holiday_item.cpp
 * @brief HolidayItem类的实现
 *
 * 该文件实现了HolidayItem类的所有方法。
 *
 * @author Sakurakugu
 * @date 2025-10-07 23:33:00(UTC+8) 周二
 */

#include "holiday_item.h"
#include <QJsonDocument>

/**
 * @brief 构造函数
 * @param date 节假日日期
 * @param name 节假日名称
 * @param isOffDay 是否为休息日
 */
HolidayItem::HolidayItem(const QDate &date, const QString &name, bool isOffDay)
    : m_date(date), m_name(name), m_isOffDay(isOffDay) {}

/**
 * @brief 从JSON对象构造
 * @param json JSON对象
 */
HolidayItem::HolidayItem(const QJsonObject &json) {
    fromJson(json);
}

/**
 * @brief 转换为JSON对象
 * @return JSON对象
 */
QJsonObject HolidayItem::toJson() const {
    QJsonObject json;
    json["date"] = m_date.toString(Qt::ISODate);
    json["name"] = m_name;
    json["isOffDay"] = m_isOffDay;
    return json;
}

/**
 * @brief 从JSON对象加载数据
 * @param json JSON对象
 * @return 是否成功
 */
bool HolidayItem::fromJson(const QJsonObject &json) {
    if (!json.contains("date") || !json.contains("name") || !json.contains("isOffDay"))
        return false;

    // 解析日期
    QString dateStr = json["date"].toString();
    m_date = QDate::fromString(dateStr, Qt::ISODate);
    if (!m_date.isValid())
        return false;

    // 解析名称
    m_name = json["name"].toString();
    if (m_name.isEmpty())
        return false;

    // 解析是否放假
    m_isOffDay = json["isOffDay"].toBool();

    return true;
}

/**
 * @brief 检查数据是否有效
 * @return 是否有效
 */
bool HolidayItem::isValid() const {
    return m_date.isValid() && !m_name.isEmpty();
}

bool HolidayItem::operator<(const HolidayItem &other) const {
    return m_date < other.m_date;
}

bool HolidayItem::operator==(const HolidayItem &other) const {
    return m_date == other.m_date && m_name == other.m_name && m_isOffDay == other.m_isOffDay;
}

bool HolidayItem::operator!=(const HolidayItem &other) const {
    return !(*this == other);
}