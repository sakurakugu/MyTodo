// ============================================================
// Time 类：时间处理类
// 提供时间的创建、操作、格式化和比较功能
//
// 作者： sakurakugu
// 创建日期： 2025-10-12 18:16:00 (UTC+8)
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
 * @brief 时间类
 *
 * 基于 std::chrono 实现的时间类，提供类型安全的时间操作
 * 表示一天中的时间（00:00:00.000 - 23:59:59.999）
 */
class Time {
  public:
    using milliseconds = std::chrono::milliseconds;
    using seconds = std::chrono::seconds;
    using minutes = std::chrono::minutes;
    using hours = std::chrono::hours;
    using duration = std::chrono::milliseconds;

    // 构造函数
    Time() = default;
    Time(int hour, int minute = 0, int second = 0, int millisecond = 0) noexcept;
    explicit Time(const duration &d) noexcept;
    explicit Time(const std::string &timeStr); // 从字符串构造 (HH:MM:SS格式)
    explicit Time(const Time &other);          // 拷贝构造函数

    // 静态工厂方法
    static Time now() noexcept;
    static Time fromString(std::string_view str) noexcept;
    static Time fromISOString(std::string_view str) noexcept;
    static Time fromMilliseconds(int64_t ms) noexcept;
    static Time fromSeconds(int64_t seconds) noexcept;
    static Time fromMinutes(int64_t minutes) noexcept;
    static Time fromHours(int64_t hours) noexcept;

    // 访问器
    int hour() const noexcept;
    int minute() const noexcept;
    int second() const noexcept;
    int millisecond() const noexcept;

    // 验证
    bool isValid() const noexcept;
    bool isAM() const noexcept;
    bool isPM() const noexcept;

    // 时间操作
    Time &addMilliseconds(int64_t ms) noexcept;
    Time &addSeconds(int64_t seconds) noexcept;
    Time &addMinutes(int64_t minutes) noexcept;
    Time &addHours(int64_t hours) noexcept;

    // 时间计算
    Time plusMilliseconds(int64_t ms) const noexcept;
    Time plusSeconds(int64_t seconds) const noexcept;
    Time plusMinutes(int64_t minutes) const noexcept;
    Time plusHours(int64_t hours) const noexcept;

    // 时间差值
    int64_t millisecondsTo(const Time &other) const noexcept;
    int64_t secondsTo(const Time &other) const noexcept;
    int64_t minutesTo(const Time &other) const noexcept;
    int64_t hoursTo(const Time &other) const noexcept;

    // 格式化
    std::string toString(std::string_view format = "hh:mm:ss") const;
    std::string toISOString() const;
    std::string to12HourString() const; // 12小时制格式 (hh:mm:ss AM/PM)
    std::string to24HourString() const; // 24小时制格式 (hh:mm:ss)

    // 转换
    duration toDuration() const noexcept;           // 转换为从午夜开始的持续时间
    int64_t toMilliseconds() const noexcept;        // 转换为毫秒数（从午夜开始）
    int64_t toSeconds() const noexcept;             // 转换为秒数（从午夜开始）
    int64_t toMinutes() const noexcept;             // 转换为分钟数（从午夜开始）
    double toHours() const noexcept;                // 转换为小时数（从午夜开始）

    // 赋值操作符
    Time &operator=(const Time &other);

    // 比较运算符
    auto operator<=>(const Time &other) const noexcept = default;
    bool operator==(const Time &other) const noexcept = default;
    bool operator!=(const Time &other) const;
    bool operator<(const Time &other) const;
    bool operator<=(const Time &other) const;
    bool operator>(const Time &other) const;
    bool operator>=(const Time &other) const;

    // 算术运算符
    Time &operator+=(const milliseconds &ms) noexcept;
    Time &operator-=(const milliseconds &ms) noexcept;
    Time &operator+=(const seconds &s) noexcept;
    Time &operator-=(const seconds &s) noexcept;
    Time &operator+=(const minutes &m) noexcept;
    Time &operator-=(const minutes &m) noexcept;
    Time &operator+=(const hours &h) noexcept;
    Time &operator-=(const hours &h) noexcept;

    Time operator+(const milliseconds &ms) const noexcept;
    Time operator-(const milliseconds &ms) const noexcept;
    Time operator+(const seconds &s) const noexcept;
    Time operator-(const seconds &s) const noexcept;
    Time operator+(const minutes &m) const noexcept;
    Time operator-(const minutes &m) const noexcept;
    Time operator+(const hours &h) const noexcept;
    Time operator-(const hours &h) const noexcept;

    duration operator-(const Time &other) const noexcept; // 两个时间相减，返回时间差

  private:
    duration m_duration{0}; // 从午夜开始的毫秒数

    // 辅助方法
    static std::optional<duration> parseISO(std::string_view str) noexcept; // 解析 ISO 格式时间字符串
    static std::optional<duration> parseCustom(std::string_view str,
                                               std::string_view format) noexcept; // 解析自定义格式时间字符串
    void normalize() noexcept; // 规范化时间（确保在0-24小时范围内）
};

} // namespace my

// std::format 支持
template <> struct std::formatter<my::Time> : std::formatter<std::string> {
    auto format(const my::Time &time, std::format_context &ctx) const {
        return std::formatter<std::string>::format(time.toISOString(), ctx);
    }
};