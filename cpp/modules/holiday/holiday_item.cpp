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

/**
 * @brief 构造函数
 * @param date 节假日日期
 * @param name 节假日名称
 * @param isOffDay 是否为休息日
 */
HolidayItem::HolidayItem(const my::Date &date, const std::string &name, bool isOffDay)
    : m_date(date), m_name(name), m_isOffDay(isOffDay) {}

/**
 * @brief 从JSON对象构造
 * @param json JSON对象
 */
HolidayItem::HolidayItem(const nlohmann::json &json) {
    fromJson(json);
}

/**
 * @brief 转换为JSON对象
 * @return JSON对象
 */
nlohmann::json HolidayItem::toJson() const {
    nlohmann::json json;
    json["date"] = m_date.toISOString();
    json["name"] = m_name;
    json["isOffDay"] = m_isOffDay;
    return json;
}

/**
 * @brief 从JSON对象加载数据
 * @param json JSON对象
 * @return 是否成功
 */
bool HolidayItem::fromJson(const nlohmann::json &json) {
    if (!json.contains("date") || !json.contains("name") || !json.contains("isOffDay"))
        return false;

    // 解析日期
    if (!json["date"].is_string())
        return false;
    std::string dateStr = json["date"].get<std::string>();
    m_date = my::Date(dateStr);
    if (!m_date.isValid())
        return false;

    // 解析名称
    if (!json["name"].is_string())
        return false;
    m_name = json["name"].get<std::string>();
    if (m_name.empty())
        return false;

    // 解析是否放假
    if (!json["isOffDay"].is_boolean())
        return false;
    m_isOffDay = json["isOffDay"].get<bool>();

    return true;
}

/**
 * @brief 检查数据是否有效
 * @return 是否有效
 */
bool HolidayItem::isValid() const {
    return m_date.isValid() && !m_name.empty();
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