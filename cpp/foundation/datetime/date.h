// ============================================================
// Date 类：日期处理类
// 提供日期的创建、操作、格式化和比较功能
//
// 作者： sakurakugu
// 创建日期： 2025-10-12 16:30:00 (UTC+8)
// ============================================================

#pragma once

#include <chrono>
#include <compare>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>

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
    Date(int year, unsigned month, unsigned day) noexcept;
    explicit Date(const sys_days &days) noexcept;
    explicit Date(const std::string &dateStr); // 从字符串构造 (YYYY-MM-DD格式)
    explicit Date(const Date &other);          // 拷贝构造函数

    // 静态工厂方法
    static Date today() noexcept;
    static Date fromString(std::string_view str) noexcept;
    static Date fromISOString(std::string_view str) noexcept;

    // 访问器
    int year() const noexcept;
    unsigned month() const noexcept;
    unsigned day() const noexcept;
    std::chrono::weekday weekday() const noexcept;
    int dayOfWeek() const;               // 返回星期几 (0=周日, 1=周一, ..., 6=周六)
    unsigned dayOfYear() const noexcept; // 返回一年中的第几天 (1-366)

    // 验证
    bool isValid() const noexcept;
    bool isLeapYear() const noexcept;

    /* 日期操作 */
    // 增加/减少时间
    Date &addDays(int days) noexcept;
    Date &addMonths(int months) noexcept;
    Date &addYears(int years) noexcept;

    // 日期计算
    Date plusDays(int days) const noexcept;
    Date plusMonths(int months) const noexcept;
    Date plusYears(int years) const noexcept;

    // 日期差值
    int daysTo(const Date &other) const noexcept;
    int monthsTo(const Date &other) const noexcept;
    int yearsTo(const Date &other) const noexcept;

    // 格式化
    std::string toString(std::string_view format = "yyyy-MM-dd") const;
    std::string toISOString() const;

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

    Date operator+(const days &d) const noexcept;   // 加天数
    Date operator-(const days &d) const noexcept;   // 减天数
    Date operator+(const months &m) const noexcept; // 加月份
    Date operator-(const months &m) const noexcept; // 减月份
    Date operator+(const years &y) const noexcept;  // 加年份
    Date operator-(const years &y) const noexcept;  // 减年份

    days operator-(const Date &other) const noexcept; // 两个日期相减，返回天数差

  private:
    year_month_day m_ymd{std::chrono::year{1900}, std::chrono::month{1}, std::chrono::day{1}};

    // 辅助方法
    static std::optional<year_month_day> parseISO(std::string_view str) noexcept; // 解析 ISO 格式日期字符串
    static std::optional<year_month_day> parseCustom(std::string_view str,
                                                     std::string_view format) noexcept; // 解析自定义格式日期字符串
};

} // namespace my

// std::format 支持
template <> struct std::formatter<my::Date> : std::formatter<std::string> {
    auto format(const my::Date &date, std::format_context &ctx) const {
        return std::formatter<std::string>::format(date.toISOString(), ctx);
    }
};