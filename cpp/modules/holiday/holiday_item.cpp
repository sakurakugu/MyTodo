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
    from_json(json, *this);
}

void to_json(nlohmann::json &j, const HolidayItem &item) {
    j = nlohmann::json{{"date", item.m_date}, {"name", item.m_name}, {"isOffDay", item.m_isOffDay}};
}

void from_json(const nlohmann::json &j, HolidayItem &item) {
    if (j.contains("date") && j["date"].is_string()) {
        item.setDate(j["date"].get<my::Date>());
    }

    if (j.contains("name") && j["name"].is_string()) {
        item.setName(j["name"].get<std::string>());
    }

    if (j.contains("isOffDay") && j["isOffDay"].is_boolean()) {
        item.setIsOffDay(j["isOffDay"].get<bool>());
    }
    // 如果包含isHoliday字段，且为布尔值，则设置isHoliday
    if (j.contains("isHoliday") && j["isHoliday"].is_boolean()) {
        item.setIsHoliday(j["isHoliday"].get<bool>());
    }
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
    return m_date == other.m_date && m_name == other.m_name && m_isOffDay == other.m_isOffDay && m_isHoliday == other.m_isHoliday;
}

bool HolidayItem::operator!=(const HolidayItem &other) const {
    return !(*this == other);
}