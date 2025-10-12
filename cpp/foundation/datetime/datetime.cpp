// ============================================================
// DateTime 类实现：日期时间处理类
//
// 作者： sakurakugu
// 创建日期： 2025-10-12 16:30:00 (UTC+8)
// ============================================================

#include "datetime.h"
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace my {

// 构造函数
DateTime::DateTime(const time_point &tp) noexcept : m_timePoint(tp) {}

DateTime::DateTime(const Date &date, int hour, int minute, int second, int millisecond) noexcept
    : m_timePoint(makeTimePoint(date.year(), date.month(), date.day(), hour, minute, second, millisecond)) {}

DateTime::DateTime(int year, unsigned month, unsigned day, int hour, int minute, int second, int millisecond) noexcept
    : m_timePoint(makeTimePoint(year, month, day, hour, minute, second, millisecond)) {}

DateTime::DateTime(const std::string& dateTimeStr) {
    auto tp = parseISO(dateTimeStr);
    if (!tp) {
        m_timePoint = time_point{};
    } else {
        m_timePoint = *tp;
    }
}

DateTime::DateTime(const DateTime& other) : m_timePoint(other.m_timePoint) {}

// 静态工厂方法
DateTime DateTime::now(TimeZone tz) noexcept {
    auto now = system_clock::now();
    return DateTime{now};
}

DateTime DateTime::utcNow() noexcept {
    return now(TimeZone::UTC);
}

DateTime DateTime::today(TimeZone tz) noexcept {
    auto now = DateTime::now(tz);
    return DateTime{now.year(), now.month(), now.day(), 0, 0, 0, 0};
}

DateTime DateTime::fromString(std::string_view str) noexcept {
    return fromISOString(str);
}

DateTime DateTime::fromISOString(std::string_view str) noexcept {
    auto tp = parseISO(str);
    if (!tp) {
        return DateTime{};
    }

    return DateTime{*tp};
}

DateTime DateTime::fromUnixTimestamp(int64_t timestamp) noexcept {
    auto tp = system_clock::from_time_t(static_cast<std::time_t>(timestamp));
    return DateTime{tp};
}

DateTime DateTime::fromUnixTimestampMs(int64_t timestampMs) noexcept {
    auto tp = system_clock::time_point{milliseconds{timestampMs}};
    return DateTime{tp};
}

// 访问器 - 日期部分
Date DateTime::date() const noexcept {
    auto days = std::chrono::floor<std::chrono::days>(m_timePoint);
    return Date{days};
}

int DateTime::year() const noexcept {
    return date().year();
}

unsigned DateTime::month() const noexcept {
    return date().month();
}

unsigned DateTime::day() const noexcept {
    return date().day();
}

std::chrono::weekday DateTime::weekday() const noexcept {
    return date().weekday();
}

int DateTime::dayOfWeek() const {
    return date().dayOfWeek();
}

unsigned DateTime::dayOfYear() const noexcept {
    return date().dayOfYear();
}

// 访问器 - 时间部分
Time DateTime::time() const noexcept {
    return Time{hour(), minute(), second(), millisecond()};
}

int DateTime::hour() const noexcept {
    auto days = std::chrono::floor<std::chrono::days>(m_timePoint);
    auto time_since_midnight = m_timePoint - days;
    auto h = std::chrono::floor<hours>(time_since_midnight);
    return static_cast<int>(h.count());
}

int DateTime::minute() const noexcept {
    auto days = std::chrono::floor<std::chrono::days>(m_timePoint);
    auto time_since_midnight = m_timePoint - days;
    auto h = std::chrono::floor<hours>(time_since_midnight);
    auto m = std::chrono::floor<minutes>(time_since_midnight - h);
    return static_cast<int>(m.count());
}

int DateTime::second() const noexcept {
    auto days = std::chrono::floor<std::chrono::days>(m_timePoint);
    auto time_since_midnight = m_timePoint - days;
    auto h = std::chrono::floor<hours>(time_since_midnight);
    auto m = std::chrono::floor<minutes>(time_since_midnight - h);
    auto s = std::chrono::floor<seconds>(time_since_midnight - h - m);
    return static_cast<int>(s.count());
}

int DateTime::millisecond() const noexcept {
    auto days = std::chrono::floor<std::chrono::days>(m_timePoint);
    auto time_since_midnight = m_timePoint - days;
    auto h = std::chrono::floor<hours>(time_since_midnight);
    auto m = std::chrono::floor<minutes>(time_since_midnight - h);
    auto s = std::chrono::floor<seconds>(time_since_midnight - h - m);
    auto ms = std::chrono::floor<milliseconds>(time_since_midnight - h - m - s);
    return static_cast<int>(ms.count());
}

// 验证
bool DateTime::isValid() const noexcept {
    return date().isValid();
}

bool DateTime::isLeapYear() const noexcept {
    return date().isLeapYear();
}

// 日期时间操作
DateTime &DateTime::addMilliseconds(int64_t ms) noexcept {
    m_timePoint += milliseconds{ms};
    return *this;
}

DateTime &DateTime::addSeconds(int64_t seconds) noexcept {
    m_timePoint += std::chrono::seconds{seconds};
    return *this;
}

DateTime &DateTime::addMinutes(int64_t minutes) noexcept {
    m_timePoint += std::chrono::minutes{minutes};
    return *this;
}

DateTime &DateTime::addHours(int64_t hours) noexcept {
    m_timePoint += std::chrono::hours{hours};
    return *this;
}

DateTime &DateTime::addDays(int days) noexcept {
    m_timePoint += std::chrono::days{days};
    return *this;
}

DateTime &DateTime::addMonths(int months) noexcept {
    auto d = date().plusMonths(months);
    auto time_part = m_timePoint - std::chrono::floor<std::chrono::days>(m_timePoint);
    m_timePoint = d.toSysDays() + time_part;
    return *this;
}

DateTime &DateTime::addYears(int years) noexcept {
    auto d = date().plusYears(years);
    auto time_part = m_timePoint - std::chrono::floor<std::chrono::days>(m_timePoint);
    m_timePoint = d.toSysDays() + time_part;
    return *this;
}

DateTime DateTime::plusMilliseconds(int64_t ms) const noexcept {
    return DateTime{*this}.addMilliseconds(ms);
}

DateTime DateTime::plusSeconds(int64_t seconds) const noexcept {
    return DateTime{*this}.addSeconds(seconds);
}

DateTime DateTime::plusMinutes(int64_t minutes) const noexcept {
    return DateTime{*this}.addMinutes(minutes);
}

DateTime DateTime::plusHours(int64_t hours) const noexcept {
    return DateTime{*this}.addHours(hours);
}

DateTime DateTime::plusDays(int days) const noexcept {
    return DateTime{*this}.addDays(days);
}

DateTime DateTime::plusMonths(int months) const noexcept {
    return DateTime{*this}.addMonths(months);
}

DateTime DateTime::plusYears(int years) const noexcept {
    return DateTime{*this}.addYears(years);
}

// 日期时间差值
int64_t DateTime::millisecondsTo(const DateTime &other) const noexcept {
    auto diff = other.m_timePoint - m_timePoint;
    return std::chrono::duration_cast<milliseconds>(diff).count();
}

int64_t DateTime::secondsTo(const DateTime &other) const noexcept {
    auto diff = other.m_timePoint - m_timePoint;
    return std::chrono::duration_cast<seconds>(diff).count();
}

int64_t DateTime::minutesTo(const DateTime &other) const noexcept {
    auto diff = other.m_timePoint - m_timePoint;
    return std::chrono::duration_cast<minutes>(diff).count();
}

int64_t DateTime::hoursTo(const DateTime &other) const noexcept {
    auto diff = other.m_timePoint - m_timePoint;
    return std::chrono::duration_cast<hours>(diff).count();
}

int DateTime::daysTo(const DateTime &other) const noexcept {
    return date().daysTo(other.date());
}

// 时间戳
int64_t DateTime::toUnixTimestamp() const noexcept {
    auto duration = m_timePoint.time_since_epoch();
    return std::chrono::duration_cast<seconds>(duration).count();
}

int64_t DateTime::toUnixTimestampMs() const noexcept {
    auto duration = m_timePoint.time_since_epoch();
    return std::chrono::duration_cast<milliseconds>(duration).count();
}

// 格式化
std::string DateTime::toString(std::string_view format) const {
    if (format == "yyyy-MM-dd hh:mm:ss" || format.empty()) {
        return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}", year(), month(), day(), hour(), minute(),
                           second());
    }

    // 简单的格式化实现
    std::string result{format};

    // 替换年份
    if (auto pos = result.find("yyyy"); pos != std::string::npos) {
        result.replace(pos, 4, std::format("{:04d}", year()));
    }

    // 替换月份
    if (auto pos = result.find("MM"); pos != std::string::npos) {
        result.replace(pos, 2, std::format("{:02d}", month()));
    }

    // 替换日期
    if (auto pos = result.find("dd"); pos != std::string::npos) {
        result.replace(pos, 2, std::format("{:02d}", day()));
    }

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

    return result;
}

std::string DateTime::toISOString() const {
    return std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.{:03d}Z", year(), month(), day(), hour(), minute(),
                       second(), millisecond());
}

std::string DateTime::toDateString() const {
    return date().toISOString();
}

std::string DateTime::toTimeString() const {
    return std::format("{:02d}:{:02d}:{:02d}", hour(), minute(), second());
}

// 转换
DateTime::time_point DateTime::toTimePoint() const noexcept {
    return m_timePoint;
}

DateTime DateTime::toUTC() const noexcept {
    // 简化实现，假设当前已经是UTC
    return *this;
}

DateTime DateTime::toLocal() const noexcept {
    // 简化实现，假设当前已经是本地时间
    return *this;
}

// 赋值操作符
DateTime& DateTime::operator=(const DateTime& other) {
    if (this != &other) {
        m_timePoint = other.m_timePoint;
    }
    return *this;
}

// 比较运算符
bool DateTime::operator!=(const DateTime& other) const {
    return m_timePoint != other.m_timePoint;
}

bool DateTime::operator<(const DateTime& other) const {
    return m_timePoint < other.m_timePoint;
}

bool DateTime::operator<=(const DateTime& other) const {
    return m_timePoint <= other.m_timePoint;
}

bool DateTime::operator>(const DateTime& other) const {
    return m_timePoint > other.m_timePoint;
}

bool DateTime::operator>=(const DateTime& other) const {
    return m_timePoint >= other.m_timePoint;
}

// 算术运算符
DateTime &DateTime::operator+=(const milliseconds &ms) noexcept {
    return addMilliseconds(ms.count());
}

DateTime &DateTime::operator-=(const milliseconds &ms) noexcept {
    return addMilliseconds(-ms.count());
}

DateTime &DateTime::operator+=(const seconds &s) noexcept {
    return addSeconds(s.count());
}

DateTime &DateTime::operator-=(const seconds &s) noexcept {
    return addSeconds(-s.count());
}

DateTime &DateTime::operator+=(const minutes &m) noexcept {
    return addMinutes(m.count());
}

DateTime &DateTime::operator-=(const minutes &m) noexcept {
    return addMinutes(-m.count());
}

DateTime &DateTime::operator+=(const hours &h) noexcept {
    return addHours(h.count());
}

DateTime &DateTime::operator-=(const hours &h) noexcept {
    return addHours(-h.count());
}

DateTime &DateTime::operator+=(const days &d) noexcept {
    return addDays(static_cast<int>(d.count()));
}

DateTime &DateTime::operator-=(const days &d) noexcept {
    return addDays(-static_cast<int>(d.count()));
}

DateTime &DateTime::operator+=(const months &m) noexcept {
    return addMonths(static_cast<int>(m.count()));
}

DateTime &DateTime::operator-=(const months &m) noexcept {
    return addMonths(-static_cast<int>(m.count()));
}

DateTime &DateTime::operator+=(const years &y) noexcept {
    return addYears(static_cast<int>(y.count()));
}

DateTime &DateTime::operator-=(const years &y) noexcept {
    return addYears(-static_cast<int>(y.count()));
}

DateTime DateTime::operator+(const milliseconds &ms) const noexcept {
    return plusMilliseconds(ms.count());
}

DateTime DateTime::operator-(const milliseconds &ms) const noexcept {
    return plusMilliseconds(-ms.count());
}

DateTime DateTime::operator+(const seconds &s) const noexcept {
    return plusSeconds(s.count());
}

DateTime DateTime::operator-(const seconds &s) const noexcept {
    return plusSeconds(-s.count());
}

DateTime DateTime::operator+(const minutes &m) const noexcept {
    return plusMinutes(m.count());
}

DateTime DateTime::operator-(const minutes &m) const noexcept {
    return plusMinutes(-m.count());
}

DateTime DateTime::operator+(const hours &h) const noexcept {
    return plusHours(h.count());
}

DateTime DateTime::operator-(const hours &h) const noexcept {
    return plusHours(-h.count());
}

DateTime DateTime::operator+(const days &d) const noexcept {
    return plusDays(static_cast<int>(d.count()));
}

DateTime DateTime::operator-(const days &d) const noexcept {
    return plusDays(-static_cast<int>(d.count()));
}

DateTime DateTime::operator+(const months &m) const noexcept {
    return plusMonths(static_cast<int>(m.count()));
}

DateTime DateTime::operator-(const months &m) const noexcept {
    return plusMonths(-static_cast<int>(m.count()));
}

DateTime DateTime::operator+(const years &y) const noexcept {
    return plusYears(static_cast<int>(y.count()));
}

DateTime DateTime::operator-(const years &y) const noexcept {
    return plusYears(-static_cast<int>(y.count()));
}

DateTime::duration DateTime::operator-(const DateTime &other) const noexcept {
    return m_timePoint - other.m_timePoint;
}

// 辅助方法
std::optional<DateTime::time_point> DateTime::parseISO(std::string_view str) noexcept {
    // 匹配 YYYY-MM-DDTHH:MM:SS.sssZ 或 YYYY-MM-DD HH:MM:SS 格式
    std::regex iso_regex(R"((\d{4})-(\d{1,2})-(\d{1,2})[T ](\d{1,2}):(\d{1,2}):(\d{1,2})(?:\.(\d{1,3}))?(?:Z|$))");
    std::string s{str};
    std::smatch match;

    if (!std::regex_match(s, match, iso_regex)) {
        return std::nullopt;
    }

    try {
        int year = std::stoi(match[1].str());
        unsigned month = std::stoul(match[2].str());
        unsigned day = std::stoul(match[3].str());
        int hour = std::stoi(match[4].str());
        int minute = std::stoi(match[5].str());
        int second = std::stoi(match[6].str());
        int millisecond = match[7].matched ? std::stoi(match[7].str()) : 0;

        if (year < 1900 || year > 3000 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 ||
            minute < 0 || minute > 59 || second < 0 || second > 59 || millisecond < 0 || millisecond > 999) {
            return std::nullopt;
        }

        return makeTimePoint(year, month, day, hour, minute, second, millisecond);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<DateTime::time_point> DateTime::parseCustom(std::string_view str, std::string_view format) noexcept {
    // 简化实现，仅支持基本格式
    return parseISO(str);
}

DateTime::time_point DateTime::makeTimePoint(int year, unsigned month, unsigned day, int hour, int minute, int second,
                                             int millisecond) noexcept {
    auto ymd = std::chrono::year_month_day{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};

    auto days = std::chrono::sys_days{ymd};
    auto time_part = hours{hour} + minutes{minute} + seconds{second} + milliseconds{millisecond};

    return days + time_part;
}

} // namespace my