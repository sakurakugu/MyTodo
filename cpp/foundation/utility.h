/**
 * @file utility.h
 * @brief Utility类的头文件
 * 该文件定义了Utility类，提供通用的实用工具函数。
 * @author Sakurakugu
 * @date 2025-09-23 18:32:17(UTC+8) 周二
 * @change 2025-09-23 18:32:17(UTC+8) 周二
 * @version 0.4.0
 */
#pragma once

#include <QDateTime>

// 定义DateType概念，限制模板参数为QDate或QDateTime
template <typename T>
concept DateType = std::same_as<T, QDate> || std::same_as<T, QDateTime>;

class Utility {
  public:
    template <typename DateType> DateType nullTime() const noexcept; // 获取空时间
    template <typename DateType> bool isNullTime(const DateType &dt) const noexcept {
        return dt == nullTime<DateType>();
    } // 检查时间是否为空
    template <typename DateType> void setNullTime(DateType &dt) const noexcept {
        dt = nullTime<DateType>();
    } // 设置时间为空
}

// ================== 模板特化 ==================
template <>
inline QDateTime Utility::nullTime<QDateTime>() const noexcept {
    return QDateTime::fromString("1970-01-01T00:00:00", Qt::ISODate);
}
template <> inline QDate Utility::nullTime<QDate>() const noexcept {
    return QDate::fromString("1970-01-01", Qt::ISODate);
}
