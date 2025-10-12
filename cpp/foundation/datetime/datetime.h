// ============================================================
// DateTime 类：日期时间处理类
// 提供日期时间的创建、操作、格式化和比较功能
//
// 作者： sakurakugu
// 创建日期： 2025-10-12 16:30:00 (UTC+8)
// ============================================================

#pragma once

#include "date.h"
#include "time.hpp"
#include <chrono>
#include <compare>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace my {

/**
 * @brief 日期时间类
 *
 * 基于 std::chrono 实现的日期时间类，提供类型安全的日期时间操作
 */
class DateTime {
  public:
    using system_clock = std::chrono::system_clock;
    using time_point = std::chrono::system_clock::time_point;
    using duration = std::chrono::system_clock::duration;
    using milliseconds = std::chrono::milliseconds;
    using seconds = std::chrono::seconds;
    using minutes = std::chrono::minutes;
    using hours = std::chrono::hours;
    using days = std::chrono::days;
    using months = std::chrono::months;
    using years = std::chrono::years;

    // 时区类型
    enum class TimeZone {
        UTC,
        Local
    };

    // 构造函数
    DateTime() = default;
    DateTime(const DateTime &other) = default;
    explicit DateTime(const time_point &tp) noexcept;
    DateTime(const Date &date, const Time &time = Time{}) noexcept;
    DateTime(const Date &date, uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0,
             uint16_t millisecond = 0) noexcept;
    DateTime(int year, uint8_t month, uint8_t day, const Time &time) noexcept;
    DateTime(int year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0,
             uint16_t millisecond = 0) noexcept;
    DateTime(const std::string &dateTimeStr); // 从字符串构造

    // 静态工厂方法
    static DateTime now(TimeZone tz = TimeZone::Local) noexcept;
    static DateTime utcNow() noexcept;
    static DateTime today(TimeZone tz = TimeZone::Local) noexcept;
    static DateTime fromString(std::string_view str) noexcept;
    static DateTime fromISOString(std::string_view str) noexcept;
    static DateTime fromUnixTimestamp(int64_t timestamp) noexcept;
    static DateTime fromUnixTimestampMs(int64_t timestampMs) noexcept;

    // 访问器 - 日期部分
    Date date() const noexcept;
    int32_t year() const noexcept;
    uint8_t month() const noexcept;
    uint8_t day() const noexcept;
    std::chrono::weekday weekday() const noexcept;
    uint8_t dayOfWeek() const noexcept;
    uint8_t dayOfYear() const noexcept;

    // 访问器 - 时间部分
    Time time() const noexcept;
    uint8_t hour() const noexcept;
    uint8_t minute() const noexcept;
    uint8_t second() const noexcept;
    uint16_t millisecond() const noexcept;

    // 验证
    bool isValid() const noexcept;
    bool isLeapYear() const noexcept;

    // 日期时间差值
    int64_t millisecondsTo(const DateTime &other) const noexcept;
    int64_t secondsTo(const DateTime &other) const noexcept;
    int64_t minutesTo(const DateTime &other) const noexcept;
    int64_t hoursTo(const DateTime &other) const noexcept;
    int daysTo(const DateTime &other) const noexcept;

    // 时间戳
    int64_t toUnixTimestamp() const noexcept;
    int64_t toUnixTimestampMs() const noexcept;

    // 格式化
    std::string toString(std::string_view format = "yyyy-MM-dd hh:mm:ss") const;
    std::string toISOString() const;
    std::string toDateString() const;
    std::string toTimeString() const;

    // 转换
    time_point toTimePoint() const noexcept;
    DateTime toUTC() const noexcept;
    DateTime toLocal() const noexcept;

    // 赋值操作符
    DateTime &operator=(const DateTime &other) = default;

    // 比较运算符
    auto operator<=>(const DateTime &other) const noexcept = default;
    bool operator==(const DateTime &other) const noexcept = default;
    bool operator!=(const DateTime &other) const;
    bool operator<(const DateTime &other) const;
    bool operator<=(const DateTime &other) const;
    bool operator>(const DateTime &other) const;
    bool operator>=(const DateTime &other) const;

    // 算术运算符
    DateTime &operator+=(const milliseconds &ms) noexcept;
    DateTime &operator-=(const milliseconds &ms) noexcept;
    DateTime &operator+=(const seconds &s) noexcept;
    DateTime &operator-=(const seconds &s) noexcept;
    DateTime &operator+=(const minutes &m) noexcept;
    DateTime &operator-=(const minutes &m) noexcept;
    DateTime &operator+=(const hours &h) noexcept;
    DateTime &operator-=(const hours &h) noexcept;
    DateTime &operator+=(const days &d) noexcept;
    DateTime &operator-=(const days &d) noexcept;
    DateTime &operator+=(const months &m) noexcept;
    DateTime &operator-=(const months &m) noexcept;
    DateTime &operator+=(const years &y) noexcept;
    DateTime &operator-=(const years &y) noexcept;

    DateTime operator+(const milliseconds &ms) const noexcept;
    DateTime operator-(const milliseconds &ms) const noexcept;
    DateTime operator+(const seconds &s) const noexcept;
    DateTime operator-(const seconds &s) const noexcept;
    DateTime operator+(const minutes &m) const noexcept;
    DateTime operator-(const minutes &m) const noexcept;
    DateTime operator+(const hours &h) const noexcept;
    DateTime operator-(const hours &h) const noexcept;
    DateTime operator+(const days &d) const noexcept;
    DateTime operator-(const days &d) const noexcept;
    DateTime operator+(const months &m) const noexcept;
    DateTime operator-(const months &m) const noexcept;
    DateTime operator+(const years &y) const noexcept;
    DateTime operator-(const years &y) const noexcept;

    duration operator-(const DateTime &other) const noexcept;

  private:
    Date m_date{};
    Time m_time{};

    // 辅助方法
    static std::optional<DateTime> parseISO(std::string_view str) noexcept;
    static std::optional<DateTime> parseCustom(std::string_view str, std::string_view format) noexcept;
    static std::chrono::minutes getUTCOffset() noexcept;
    void normalizeDateTime() noexcept; // 处理跨日期边界

    // 日期时间操作
    DateTime &addMilliseconds(int64_t ms) noexcept;
    DateTime &addSeconds(int64_t seconds) noexcept;
    DateTime &addMinutes(int64_t minutes) noexcept;
    DateTime &addHours(int64_t hours) noexcept;
    DateTime &addDays(int32_t days) noexcept;
    DateTime &addMonths(int32_t months) noexcept;
    DateTime &addYears(int32_t years) noexcept;

    DateTime plusMilliseconds(int64_t ms) const noexcept;
    DateTime plusSeconds(int64_t seconds) const noexcept;
    DateTime plusMinutes(int64_t minutes) const noexcept;
    DateTime plusHours(int64_t hours) const noexcept;
    DateTime plusDays(int32_t days) const noexcept;
    DateTime plusMonths(int32_t months) const noexcept;
    DateTime plusYears(int32_t years) const noexcept;
};

} // namespace my

// std::format 支持
template <> struct std::formatter<my::DateTime> : std::formatter<std::string> {
    auto format(const my::DateTime &dt, std::format_context &ctx) const {
        return std::formatter<std::string>::format(dt.toISOString(), ctx);
    }
};