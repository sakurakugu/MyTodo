// ============================================================
// Time 类实现：时间处理类
//
// 作者： sakurakugu
// 创建日期： 2025-10-12 18:16:00 (UTC+8)
// ============================================================

#include "time.h"
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace my {

// 构造函数
Time::Time(int hour, int minute, int second, int millisecond) noexcept {
    m_duration = hours{hour} + minutes{minute} + seconds{second} + milliseconds{millisecond};
    normalize();
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

Time::Time(const Time &other) : m_duration(other.m_duration) {}

// 静态工厂方法
Time Time::now() noexcept {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    auto ms = std::chrono::duration_cast<milliseconds>(
        now.time_since_epoch() % std::chrono::seconds{1}
    ).count();
    
    return Time{tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<int>(ms)};
}

Time Time::fromString(std::string_view str) noexcept {
    return fromISOString(str);
}

Time Time::fromISOString(std::string_view str) noexcept {
    auto d = parseISO(str);
    if (!d) {
        return Time{};
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

// 访问器
int Time::hour() const noexcept {
    auto h = std::chrono::duration_cast<hours>(m_duration);
    return static_cast<int>(h.count());
}

int Time::minute() const noexcept {
    auto h = std::chrono::duration_cast<hours>(m_duration);
    auto m = std::chrono::duration_cast<minutes>(m_duration - h);
    return static_cast<int>(m.count());
}

int Time::second() const noexcept {
    auto h = std::chrono::duration_cast<hours>(m_duration);
    auto m = std::chrono::duration_cast<minutes>(m_duration - h);
    auto s = std::chrono::duration_cast<seconds>(m_duration - h - m);
    return static_cast<int>(s.count());
}

int Time::millisecond() const noexcept {
    auto h = std::chrono::duration_cast<hours>(m_duration);
    auto m = std::chrono::duration_cast<minutes>(m_duration - h);
    auto s = std::chrono::duration_cast<seconds>(m_duration - h - m);
    auto ms = std::chrono::duration_cast<milliseconds>(m_duration - h - m - s);
    return static_cast<int>(ms.count());
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
    if (format == "hh:mm:ss" || format.empty()) {
        return to24HourString();
    }
    
    // 简单的格式化实现
    std::string result{format};
    
    // 替换小时
    if (auto pos = result.find("hh"); pos != std::string::npos) {
        result.replace(pos, 2, std::format("{:02d}", hour()));
    }
    
    // 替换分钟
    if (auto pos = result.find("mm"); pos != std::string::npos) {
        result.replace(pos, 2, std::format("{:02d}", minute()));
    }
    
    // 替换秒
    if (auto pos = result.find("ss"); pos != std::string::npos) {
        result.replace(pos, 2, std::format("{:02d}", second()));
    }
    
    // 替换毫秒
    if (auto pos = result.find("fff"); pos != std::string::npos) {
        result.replace(pos, 3, std::format("{:03d}", millisecond()));
    }
    
    return result;
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

Time::duration Time::operator-(const Time &other) const noexcept {
    return m_duration - other.m_duration;
}

// 辅助方法
std::optional<Time::duration> Time::parseISO(std::string_view str) noexcept {
    // 匹配 HH:MM:SS.sss 或 HH:MM:SS 格式
    std::regex iso_regex(R"((\d{1,2}):(\d{1,2}):(\d{1,2})(?:\.(\d{1,3}))?)");
    std::string s{str};
    std::smatch match;
    
    if (!std::regex_match(s, match, iso_regex)) {
        return std::nullopt;
    }
    
    try {
        int hour = std::stoi(match[1].str());
        int minute = std::stoi(match[2].str());
        int second = std::stoi(match[3].str());
        int millisecond = match[4].matched ? std::stoi(match[4].str()) : 0;
        
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || 
            second < 0 || second > 59 || millisecond < 0 || millisecond > 999) {
            return std::nullopt;
        }
        
        return hours{hour} + minutes{minute} + seconds{second} + milliseconds{millisecond};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Time::duration> Time::parseCustom(std::string_view str, std::string_view format) noexcept {
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

} // namespace my