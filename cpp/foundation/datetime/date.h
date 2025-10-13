/**
 * @file date.h
 * @brief 日期处理类
 *
 * (自动生成的头部；请完善 @brief 及详细描述)
 * @author sakurakugu
 * @date 2025-10-12 16:30:00 (UTC+8)
 */

#pragma once

#include <chrono>
#include <compare>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>

// Qt 前向声明
#ifdef QT_CORE_LIB
class QDate;
#endif

namespace my {

/**
 * @brief 日期类
 *
 * 基于 std::chrono 实现的日期类，提供类型安全的日期操作
 */
class Date {
  public:
    using days = std::chrono::days;
    using months = std::chrono::months;
    using years = std::chrono::years;
    using year_month_day = std::chrono::year_month_day;
    using sys_days = std::chrono::sys_days;

    // 构造函数
    Date() = default;
    explicit Date(const year_month_day &ymd) noexcept;
    Date(int32_t year, uint8_t month, uint8_t day) noexcept;
    explicit Date(const sys_days &days) noexcept;
    explicit Date(const std::string &dateStr); // 从字符串构造 (YYYY-MM-DD格式)
    Date(const Date &other);                   // 拷贝构造函数
#ifdef QT_CORE_LIB
    explicit Date(const QDate &qdate) noexcept;
#endif

    // 静态工厂方法
    static Date today() noexcept;
    static Date fromString(std::string_view str) noexcept;
    static Date fromISOString(std::string_view str) noexcept;
#ifdef QT_CORE_LIB
    static Date fromQDate(const QDate &qdate) noexcept;
#endif

    // 访问器
    int32_t year() const noexcept;
    uint8_t month() const noexcept;
    uint8_t day() const noexcept;
    std::chrono::weekday weekday() const noexcept;
    uint8_t dayOfWeek() const noexcept; // 返回星期几 (0=周日, 1=周一, ..., 6=周六)
    uint8_t dayOfYear() const noexcept; // 返回一年中的第几天 (1-366)

    // 验证
    bool isValid() const noexcept;
    bool isLeapYear() const noexcept; // 是否闰年

    /* 日期操作 */
    // 增加/减少时间
    Date &addDays(int32_t days) noexcept;
    Date &addMonths(int32_t months) noexcept;
    Date &addYears(int32_t years) noexcept;

    // 日期计算
    Date plusDays(int32_t days) const noexcept;
    Date plusMonths(int32_t months) const noexcept;
    Date plusYears(int32_t years) const noexcept;

    // 日期差值
    int32_t daysTo(const Date &other) const noexcept;
    int32_t monthsTo(const Date &other) const noexcept;
    int32_t yearsTo(const Date &other) const noexcept;

    // 格式化
    std::string toString(std::string_view format = "yyyy-MM-dd") const;
    std::string toISOString() const;
#ifdef QT_CORE_LIB
    QDate toQDate() const noexcept;
#endif

    // 转换
    sys_days toSysDays() const noexcept;            // 转换为 sys_days 类型
    year_month_day toYearMonthDay() const noexcept; // 转换为 year_month_day 类型

    // 赋值操作符
    Date &operator=(const Date &other);

    // 比较运算符
    auto operator<=>(const Date &other) const noexcept = default;
    bool operator==(const Date &other) const noexcept = default;
    bool operator!=(const Date &other) const;
    bool operator<(const Date &other) const;
    bool operator<=(const Date &other) const;
    bool operator>(const Date &other) const;
    bool operator>=(const Date &other) const;

    // 算术运算符
    Date &operator+=(const days &d) noexcept;
    Date &operator-=(const days &d) noexcept;
    Date &operator+=(const months &m) noexcept;
    Date &operator-=(const months &m) noexcept;
    Date &operator+=(const years &y) noexcept;
    Date &operator-=(const years &y) noexcept;
    Date &operator+=(const Date &other) noexcept;
    Date &operator-=(const Date &other) noexcept;

    Date operator+(const days &d) const noexcept;     // 加天数
    Date operator-(const days &d) const noexcept;     // 减天数
    Date operator+(const months &m) const noexcept;   // 加月份
    Date operator-(const months &m) const noexcept;   // 减月份
    Date operator+(const years &y) const noexcept;    // 加年份
    Date operator-(const years &y) const noexcept;    // 减年份
    Date operator+(const Date &other) const noexcept; // 加日期
    Date operator-(const Date &other) const noexcept; // 减日期

  private:
    year_month_day m_ymd{std::chrono::year{1900}, std::chrono::month{1}, std::chrono::day{1}};

    // 辅助方法
    static std::optional<year_month_day> parseISO(std::string_view str) noexcept; // 解析 ISO 格式日期字符串
    static std::optional<year_month_day> parseCustom(std::string_view str,
                                                     std::string_view format) noexcept; // 解析自定义格式日期字符串
};

// << 运算符重载
inline std::ostream &operator<<(std::ostream &os, const my::Date &date) {
    os << date.toISOString();
    return os;
}

} // namespace my

// std::format 支持
template <> struct std::formatter<my::Date> : std::formatter<std::string> {
    auto format(const my::Date &date, std::format_context &ctx) const {
        return std::formatter<std::string>::format(date.toISOString(), ctx);
    }
};
