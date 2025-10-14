/**
 * @file time.cpp
 * @brief 时间处理类实现
 *
 * (自动生成的头部；请完善 @brief 及详细描述)
 * @author sakurakugu
 * @date 2025-10-12 18:16:00 (UTC+8)
 */

#include "time.hpp"
#include "formatter.h"
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

#ifdef QT_CORE_LIB
#include <QTime>
#endif

namespace my {

// 构造函数
Time::Time(uint8_t hour, uint8_t minute, uint8_t second, uint16_t millisecond) noexcept {
    // 检查参数有效性
    if (hour > 23 || minute > 59 || second > 59 || millisecond > 999) {
        // 创建无效时间对象，使用负值表示无效状态
        m_duration = duration{-1};
    } else {
        m_duration = hours{hour} + minutes{minute} + seconds{second} + milliseconds{millisecond};
        normalize();
    }
}

Time::Time(const duration &d) noexcept : m_duration(d) {
    normalize();
}

Time::Time(const std::string &timeStr) {
    auto d = parseISO(timeStr);
    if (!d) {
        m_duration = duration{0};
    } else {
        m_duration = *d;
        normalize();
    }
}

// 从 ClockTime 构造
Time::Time(const ClockTime &ct) noexcept : Time(ct.hour, ct.minute, ct.second, ct.millisecond) {}

Time::Time(const Time &other) : m_duration(other.m_duration) {}

// 静态工厂方法
Time Time::now() noexcept {
    auto now = std::chrono::system_clock::now();

    // 使用现代 C++ chrono 库，线程安全
    auto local_time = std::chrono::current_zone()->to_local(now); // 转换为本地时间
    auto days = std::chrono::floor<std::chrono::days>(local_time);
    auto time_of_day = local_time - days;

    auto h = duration_cast<hours>(time_of_day);
    auto m = duration_cast<minutes>(time_of_day - h);
    auto s = duration_cast<seconds>(time_of_day - h - m);
    auto ms = duration_cast<milliseconds>(time_of_day - h - m - s);

    return Time{static_cast<uint8_t>(h.count()), //
                static_cast<uint8_t>(m.count()), //
                static_cast<uint8_t>(s.count()), //
                static_cast<uint16_t>(ms.count())};
}

#ifdef NLOHMANN_JSON_ENABLED
// 序列化
void to_json(nlohmann::json &j, const Time &time) {
    if (time.isValid()) {
        j = time.toISOString();
    } else {
        j = nullptr;
    }
}

// 反序列化
void from_json(const nlohmann::json &j, Time &time) {
    if (j.is_string()) {
        time = Time{j.get<std::string>()};
    } else {
        time = Time{};
    }
}
#endif

Time Time::fromString(std::string_view str) noexcept {
    // TODO: 支持自定义格式
    return fromISOString(str);
}

Time Time::fromISOString(std::string_view str) noexcept {
    auto d = parseISO(str);
    if (!d) {
        // 创建无效时间对象
        Time invalid;
        invalid.m_duration = duration{-1};
        return invalid;
    }
    return Time{*d};
}

Time Time::fromMilliseconds(int64_t ms) noexcept {
    return Time{milliseconds{ms}};
}

Time Time::fromSeconds(int64_t seconds) noexcept {
    return Time{std::chrono::seconds{seconds}};
}

Time Time::fromMinutes(int64_t minutes) noexcept {
    return Time{std::chrono::minutes{minutes}};
}

Time Time::fromHours(int64_t hours) noexcept {
    return Time{std::chrono::hours{hours}};
}

// 一次性计算所有组件
ClockTime Time::getComponents() const noexcept {
    int64_t total_ms = std::chrono::duration_cast<milliseconds>(m_duration).count();

    // 处理负值（无效时间）
    if (total_ms < 0) {
        return ClockTime{0, 0, 0, 0};
    }

    // 保证只在一天范围内
    total_ms %= 24 * 60 * 60 * 1000;

    uint8_t h = static_cast<uint8_t>(total_ms / (60 * 60 * 1000));
    total_ms %= (60 * 60 * 1000);

    uint8_t m = static_cast<uint8_t>(total_ms / (60 * 1000));
    total_ms %= (60 * 1000);

    uint8_t s = static_cast<uint8_t>(total_ms / 1000);
    uint16_t ms = static_cast<uint16_t>(total_ms % 1000);

    return ClockTime{h, m, s, ms};
}

// 访问器
int Time::hour() const noexcept {
    return getComponents().hour;
}

int Time::minute() const noexcept {
    return getComponents().minute;
}

int Time::second() const noexcept {
    return getComponents().second;
}

int Time::millisecond() const noexcept {
    return getComponents().millisecond;
}

// 验证
bool Time::isValid() const noexcept {
    return m_duration >= duration{0} && m_duration < hours{24};
}

bool Time::isAM() const noexcept {
    return hour() < 12;
}

bool Time::isPM() const noexcept {
    return hour() >= 12;
}

// 时间操作
Time &Time::addMilliseconds(int64_t ms) noexcept {
    m_duration += milliseconds{ms};
    normalize();
    return *this;
}

Time &Time::addSeconds(int64_t seconds) noexcept {
    m_duration += std::chrono::seconds{seconds};
    normalize();
    return *this;
}

Time &Time::addMinutes(int64_t minutes) noexcept {
    m_duration += std::chrono::minutes{minutes};
    normalize();
    return *this;
}

Time &Time::addHours(int64_t hours) noexcept {
    m_duration += std::chrono::hours{hours};
    normalize();
    return *this;
}

// 时间计算
Time Time::plusMilliseconds(int64_t ms) const noexcept {
    return Time{*this}.addMilliseconds(ms);
}

Time Time::plusSeconds(int64_t seconds) const noexcept {
    return Time{*this}.addSeconds(seconds);
}

Time Time::plusMinutes(int64_t minutes) const noexcept {
    return Time{*this}.addMinutes(minutes);
}

Time Time::plusHours(int64_t hours) const noexcept {
    return Time{*this}.addHours(hours);
}

// 时间差值
int64_t Time::millisecondsTo(const Time &other) const noexcept {
    return std::chrono::duration_cast<milliseconds>(other.m_duration - m_duration).count();
}

int64_t Time::secondsTo(const Time &other) const noexcept {
    return std::chrono::duration_cast<seconds>(other.m_duration - m_duration).count();
}

int64_t Time::minutesTo(const Time &other) const noexcept {
    return std::chrono::duration_cast<minutes>(other.m_duration - m_duration).count();
}

int64_t Time::hoursTo(const Time &other) const noexcept {
    return std::chrono::duration_cast<hours>(other.m_duration - m_duration).count();
}

// 格式化
std::string Time::toString(std::string_view format) const {
    if (format.empty()) {
        // 默认返回24小时制格式，不包含毫秒
        return std::format("{:02d}:{:02d}:{:02d}", hour(), minute(), second());
    }

    // 使用统一的格式化工具
    auto replacements = DateTimeFormatter::createTimeReplacements(hour(), minute(), second(), millisecond());
    return DateTimeFormatter::format(format, replacements);
}

std::string Time::toISOString() const {
    return std::format("{:02d}:{:02d}:{:02d}.{:03d}", hour(), minute(), second(), millisecond());
}

std::string Time::to12HourString() const {
    int h = hour();
    std::string ampm = isAM() ? "AM" : "PM";

    if (h == 0) {
        h = 12; // 午夜显示为12 AM
    } else if (h > 12) {
        h -= 12; // 下午时间转换为12小时制
    }

    return std::format("{:02d}:{:02d}:{:02d} {}", h, minute(), second(), ampm);
}

std::string Time::to24HourString() const {
    return std::format("{:02d}:{:02d}:{:02d}", hour(), minute(), second());
}

// 转换
Time::duration Time::toDuration() const noexcept {
    return m_duration;
}

int64_t Time::toMilliseconds() const noexcept {
    return std::chrono::duration_cast<milliseconds>(m_duration).count();
}

int64_t Time::toSeconds() const noexcept {
    return std::chrono::duration_cast<seconds>(m_duration).count();
}

int64_t Time::toMinutes() const noexcept {
    return std::chrono::duration_cast<minutes>(m_duration).count();
}

double Time::toHours() const noexcept {
    return std::chrono::duration_cast<std::chrono::duration<double, std::ratio<3600>>>(m_duration).count();
}

// 赋值操作符
Time &Time::operator=(const Time &other) {
    if (this != &other) {
        m_duration = other.m_duration;
    }
    return *this;
}

// 比较运算符
bool Time::operator!=(const Time &other) const {
    return m_duration != other.m_duration;
}

bool Time::operator<(const Time &other) const {
    return m_duration < other.m_duration;
}

bool Time::operator<=(const Time &other) const {
    return m_duration <= other.m_duration;
}

bool Time::operator>(const Time &other) const {
    return m_duration > other.m_duration;
}

bool Time::operator>=(const Time &other) const {
    return m_duration >= other.m_duration;
}

// 算术运算符
Time &Time::operator+=(const milliseconds &ms) noexcept {
    return addMilliseconds(ms.count());
}

Time &Time::operator-=(const milliseconds &ms) noexcept {
    return addMilliseconds(-ms.count());
}

Time &Time::operator+=(const seconds &s) noexcept {
    return addSeconds(s.count());
}

Time &Time::operator-=(const seconds &s) noexcept {
    return addSeconds(-s.count());
}

Time &Time::operator+=(const minutes &m) noexcept {
    return addMinutes(m.count());
}

Time &Time::operator-=(const minutes &m) noexcept {
    return addMinutes(-m.count());
}

Time &Time::operator+=(const hours &h) noexcept {
    return addHours(h.count());
}

Time &Time::operator-=(const hours &h) noexcept {
    return addHours(-h.count());
}

Time &Time::operator+=(const Time &other) noexcept {
    return addMilliseconds(other.toMilliseconds());
}

Time &Time::operator-=(const Time &other) noexcept {
    return addMilliseconds(-other.toMilliseconds());
}

Time Time::operator+(const milliseconds &ms) const noexcept {
    return plusMilliseconds(ms.count());
}

Time Time::operator-(const milliseconds &ms) const noexcept {
    return plusMilliseconds(-ms.count());
}

Time Time::operator+(const seconds &s) const noexcept {
    return plusSeconds(s.count());
}

Time Time::operator-(const seconds &s) const noexcept {
    return plusSeconds(-s.count());
}

Time Time::operator+(const minutes &m) const noexcept {
    return plusMinutes(m.count());
}

Time Time::operator-(const minutes &m) const noexcept {
    return plusMinutes(-m.count());
}

Time Time::operator+(const hours &h) const noexcept {
    return plusHours(h.count());
}

Time Time::operator-(const hours &h) const noexcept {
    return plusHours(-h.count());
}

Time Time::operator+(const Time &other) const noexcept {
    return plusMilliseconds(other.toMilliseconds());
}

Time Time::operator-(const Time &other) const noexcept {
    return plusMilliseconds(-other.toMilliseconds());
}

// 辅助方法
std::optional<Time::duration> Time::parseISO(std::string_view str) noexcept {
    // 匹配 HH:MM:SS.sss 或 HH:MM:SS 格式
    static const std::regex iso_regex(R"((\d{1,2}):(\d{1,2}):(\d{1,2})(?:\.(\d{1,3}))?)");
    std::string s{str};
    std::smatch match;

    if (!std::regex_match(s, match, iso_regex)) {
        return std::nullopt;
    }

    try {
        uint8_t hour = std::stoi(match[1].str());
        uint8_t minute = std::stoi(match[2].str());
        uint8_t second = std::stoi(match[3].str());
        uint16_t millisecond = match[4].matched ? std::stoi(match[4].str()) : 0;

        if (hour > 23 || minute > 59 || second > 59 || millisecond > 999) {
            return std::nullopt;
        }

        return hours{hour} + minutes{minute} + seconds{second} + milliseconds{millisecond};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Time::duration> Time::parseCustom(std::string_view str,
                                                [[maybe_unused]] std::string_view format) noexcept {
    // 简化实现，仅支持基本格式
    return parseISO(str);
}

void Time::normalize() noexcept {
    // 确保时间在0-24小时范围内
    auto day_duration = hours{24};

    // 处理负数时间
    while (m_duration < duration{0}) {
        m_duration += day_duration;
    }

    // 处理超过24小时的时间
    while (m_duration >= day_duration) {
        m_duration -= day_duration;
    }
}

#ifdef QT_CORE_LIB

// Qt 转换构造函数
Time::Time(const QTime &qtime) noexcept {
    if (qtime.isValid()) {
        m_duration =
            hours{qtime.hour()} + minutes{qtime.minute()} + seconds{qtime.second()} + milliseconds{qtime.msec()};
    } else {
        m_duration = duration{0};
    }
    normalize();
}

// Qt 转换静态方法
Time Time::fromQTime(const QTime &qtime) noexcept {
    return Time{qtime};
}

// Qt 转换方法
QTime Time::toQTime() const noexcept {
    if (!isValid()) {
        return QTime{};
    }
    auto components = getComponents();
    return QTime{components.hour, components.minute, components.second, components.millisecond};
}
#endif

} // namespace my