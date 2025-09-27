/**
 * @file utility.h
 * @brief Utility类的头文件
 * 该文件定义了Utility类，提供通用的实用工具函数。
 * @author Sakurakugu
 * @date 2025-09-23 18:32:17(UTC+8) 周二
 * @change 2025-09-24 00:55:58(UTC+8) 周三
 */
#pragma once

#include <QDateTime>

// 定义DateType概念，限制模板参数为QDate或QDateTime
template <typename T>
concept DateType = std::same_as<T, QDate> || std::same_as<T, QDateTime>;

class Utility {
  public:
    template <typename DateType> static DateType nullTime() noexcept;                 // 获取空时间
    template <typename DateType> static bool isNullTime(const DateType &dt) noexcept; // 检查时间是否为空
    template <typename DateType> static void setNullTime(DateType &dt) noexcept;      // 设置时间为空

  private:
    Utility() = default;
    ~Utility() noexcept = default;
};

// 模板函数实现移到类外部
template <typename DateType> bool Utility::isNullTime(const DateType &dt) noexcept {
    return dt == nullTime<DateType>();
} // 检查时间是否为空

template <typename DateType> void Utility::setNullTime(DateType &dt) noexcept {
    dt = nullTime<DateType>();
} // 设置时间为空

// ================== 模板特化 ==================
template <> inline QDateTime Utility::nullTime<QDateTime>() noexcept {
    return QDateTime{}; // 已经改成时间戳了
}

template <> inline QDate Utility::nullTime<QDate>() noexcept {
    return QDate{}; // 已经改成时间戳了
}
