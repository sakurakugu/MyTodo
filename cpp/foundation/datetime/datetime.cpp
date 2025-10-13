/**
 * @file datetime.cpp
 * @brief 日期时间处理类实现
 *
 * (自动生成的头部；请完善 @brief 及详细描述)
 * @author sakurakugu
 * @date 2025-10-12 16:30:00 (UTC+8)
 */

#include "datetime.h"
#include "formatter.h"
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef QT_CORE_LIB
#include <QDateTime>
#endif

namespace my {

// 构造函数
DateTime::DateTime(const time_point &tp) noexcept {
    auto days = std::chrono::floor<std::chrono::days>(tp);
    auto time_since_midnight = tp - days;

    auto h = std::chrono::floor<std::chrono::hours>(time_since_midnight);
    auto m = std::chrono::floor<std::chrono::minutes>(time_since_midnight - h);
    auto s = std::chrono::floor<std::chrono::seconds>(time_since_midnight - h - m);
    auto ms = std::chrono::floor<std::chrono::milliseconds>(time_since_midnight - h - m - s);

    m_date = Date{days};
    m_time = Time{static_cast<uint8_t>(h.count()), static_cast<uint8_t>(m.count()), static_cast<uint8_t>(s.count()),
                  static_cast<uint16_t>(ms.count())};
}

DateTime::DateTime(const Date &date, const Time &time) noexcept : m_date(date), m_time(time) {}
DateTime::DateTime(int32_t year, uint8_t month, uint8_t day, const Time &time) noexcept
    : m_date(year, month, day), m_time(time) {}

DateTime::DateTime(const Date &date, uint8_t hour, uint8_t minute, uint8_t second, uint16_t millisecond) noexcept
    : m_date(date), m_time(hour, minute, second, millisecond) {}

DateTime::DateTime(int32_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second,
                   uint16_t millisecond) noexcept
    : m_date(year, month, day), m_time(hour, minute, second, millisecond) {}

DateTime::DateTime(const std::string &dateTimeStr) {
    auto dt = parseISO(dateTimeStr);
    if (!dt) {
        m_date = Date{};
        m_time = Time{};
    } else {
        m_date = dt->m_date;
        m_time = dt->m_time;
    }
}

// 静态工厂方法
/**
 * @brief 获取当前日期时间
 *
 * @param tz 时区，默认本地时区
 * @return DateTime 当前日期时间
 */
DateTime DateTime::now(TimeZone tz) noexcept {
    // TODO: 实现时区转换
    auto now = system_clock::now();
    if (tz == TimeZone::UTC) {
        return DateTime{now};
    } else {
        return DateTime{now + getUTCOffset()};
    }
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
    return m_date;
}

int32_t DateTime::year() const noexcept {
    return m_date.year();
}

uint8_t DateTime::month() const noexcept {
    return m_date.month();
}

uint8_t DateTime::day() const noexcept {
    return m_date.day();
}

std::chrono::weekday DateTime::weekday() const noexcept {
    return m_date.weekday();
}

uint8_t DateTime::dayOfWeek() const noexcept {
    return m_date.dayOfWeek();
}

uint8_t DateTime::dayOfYear() const noexcept {
    return m_date.dayOfYear();
}

// 访问器 - 时间部分
Time DateTime::time() const noexcept {
    return m_time;
}

uint8_t DateTime::hour() const noexcept {
    return m_time.hour();
}

uint8_t DateTime::minute() const noexcept {
    return m_time.minute();
}

uint8_t DateTime::second() const noexcept {
    return m_time.second();
}

uint16_t DateTime::millisecond() const noexcept {
    return m_time.millisecond();
}

// 验证
bool DateTime::isValid() const noexcept {
    return m_date.isValid() && m_time.isValid();
}

bool DateTime::isLeapYear() const noexcept {
    return m_date.isLeapYear();
}

// 日期时间操作
DateTime &DateTime::addMilliseconds(int64_t ms) noexcept {
    m_time += milliseconds{ms};
    normalizeDateTime();
    return *this;
}

DateTime &DateTime::addSeconds(int64_t seconds) noexcept {
    m_time.addSeconds(seconds);
    normalizeDateTime();
    return *this;
}

DateTime &DateTime::addMinutes(int64_t minutes) noexcept {
    m_time.addMinutes(minutes);
    normalizeDateTime();
    return *this;
}

DateTime &DateTime::addHours(int64_t hours) noexcept {
    m_time.addHours(hours);
    normalizeDateTime();
    return *this;
}

DateTime &DateTime::addDays(int days) noexcept {
    m_date.addDays(days);
    return *this;
}

DateTime &DateTime::addMonths(int months) noexcept {
    m_date.addMonths(months);
    return *this;
}

DateTime &DateTime::addYears(int years) noexcept {
    m_date.addYears(years);
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
    auto diff = other.toTimePoint() - toTimePoint();
    return std::chrono::duration_cast<milliseconds>(diff).count();
}

int64_t DateTime::secondsTo(const DateTime &other) const noexcept {
    auto diff = other.toTimePoint() - toTimePoint();
    return std::chrono::duration_cast<seconds>(diff).count();
}

int64_t DateTime::minutesTo(const DateTime &other) const noexcept {
    auto diff = other.toTimePoint() - toTimePoint();
    return std::chrono::duration_cast<minutes>(diff).count();
}

int64_t DateTime::hoursTo(const DateTime &other) const noexcept {
    auto diff = other.toTimePoint() - toTimePoint();
    return std::chrono::duration_cast<hours>(diff).count();
}

int DateTime::daysTo(const DateTime &other) const noexcept {
    return date().daysTo(other.date());
}

// 时间戳
int64_t DateTime::toUnixTimestamp() const noexcept {
    auto duration = toTimePoint().time_since_epoch();
    return std::chrono::duration_cast<seconds>(duration).count();
}

int64_t DateTime::toUnixTimestampMs() const noexcept {
    auto duration = toTimePoint().time_since_epoch();
    return std::chrono::duration_cast<milliseconds>(duration).count();
}

// 格式化
std::string DateTime::toString(std::string_view format) const {
    if (format.empty()) {
        return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}", year(), month(), day(), hour(), minute(),
                           second());
    }

    // 使用统一的格式化工具
    auto replacements = DateTimeFormatter::createDateTimeReplacements(year(), month(), day(), hour(), minute(),
                                                                      second(), millisecond());
    return DateTimeFormatter::format(format, replacements);
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
std::chrono::system_clock::time_point DateTime::toTimePoint() const noexcept {
    auto days = m_date.toSysDays();
    auto time_duration = std::chrono::hours{m_time.hour()} + std::chrono::minutes{m_time.minute()} +
                         std::chrono::seconds{m_time.second()} + std::chrono::milliseconds{m_time.millisecond()};
    return days + time_duration;
}

// 处理跨日期边界的私有方法
void DateTime::normalizeDateTime() noexcept {
    // 如果时间超过了一天的范围，需要调整日期
    int32_t totalMilliseconds =
        m_time.hour() * 3600000 + m_time.minute() * 60000 + m_time.second() * 1000 + m_time.millisecond();

    if (totalMilliseconds >= 86400000) { // 超过一天
        int32_t days = totalMilliseconds / 86400000;
        m_date.addDays(days);
        totalMilliseconds %= 86400000;

        uint8_t hours = totalMilliseconds / 3600000;
        totalMilliseconds %= 3600000;
        uint8_t minutes = totalMilliseconds / 60000;
        totalMilliseconds %= 60000;
        uint8_t seconds = totalMilliseconds / 1000;
        uint16_t milliseconds = totalMilliseconds % 1000;

        m_time = Time{hours, minutes, seconds, milliseconds};
    } else if (totalMilliseconds < 0) { // 小于零
        int32_t days = (-totalMilliseconds - 1) / 86400000 + 1;
        m_date.addDays(-days);
        totalMilliseconds += days * 86400000;

        uint8_t hours = totalMilliseconds / 3600000;
        totalMilliseconds %= 3600000;
        uint8_t minutes = totalMilliseconds / 60000;
        totalMilliseconds %= 60000;
        uint8_t seconds = totalMilliseconds / 1000;
        uint16_t milliseconds = totalMilliseconds % 1000;

        m_time = Time{hours, minutes, seconds, milliseconds};
    }
}

DateTime DateTime::toUTC() const noexcept {
    try {
        // 假设当前时间是本地时间，转换为 UTC
        auto timePoint = toTimePoint();
        auto local_time = std::chrono::current_zone()->to_local(timePoint);
        auto utc_time = std::chrono::current_zone()->to_sys(local_time);

        auto days = std::chrono::floor<std::chrono::days>(utc_time);
        auto time_since_midnight = utc_time - days;

        auto h = std::chrono::floor<std::chrono::hours>(time_since_midnight);
        auto m = std::chrono::floor<std::chrono::minutes>(time_since_midnight - h);
        auto s = std::chrono::floor<std::chrono::seconds>(time_since_midnight - h - m);
        auto ms = std::chrono::floor<std::chrono::milliseconds>(time_since_midnight - h - m - s);

        Date utcDate{days};
        Time utcTime{static_cast<uint8_t>(h.count()), //
                     static_cast<uint8_t>(m.count()), //
                     static_cast<uint8_t>(s.count()), //
                     static_cast<uint16_t>(ms.count())};

        return DateTime{utcDate, utcTime};
    } catch (...) {
        // 如果转换失败，返回原始时间
        return *this;
    }
}

DateTime DateTime::toLocal() const noexcept {
    try {
        // 假设当前时间是 UTC，转换为本地时间
        auto timePoint = toTimePoint();
        auto local_time = std::chrono::current_zone()->to_local(timePoint);

        auto days = std::chrono::floor<std::chrono::days>(local_time);
        auto time_since_midnight = local_time - days;

        auto h = std::chrono::floor<std::chrono::hours>(time_since_midnight);
        auto m = std::chrono::floor<std::chrono::minutes>(time_since_midnight - h);
        auto s = std::chrono::floor<std::chrono::seconds>(time_since_midnight - h - m);
        auto ms = std::chrono::floor<std::chrono::milliseconds>(time_since_midnight - h - m - s);

        // 将本地日期转换为系统日期以便用于日期构造函数中
        auto sys_days = std::chrono::sys_days{days.time_since_epoch()};
        Date localDate{sys_days};
        Time localTime{static_cast<uint8_t>(h.count()), //
                       static_cast<uint8_t>(m.count()), //
                       static_cast<uint8_t>(s.count()), //
                       static_cast<uint16_t>(ms.count())};

        return DateTime{localDate, localTime};
    } catch (...) {
        // 如果转换失败，返回原始时间
        return *this;
    }
}

bool DateTime::operator==(const DateTime &other) const noexcept {
    return m_date == other.m_date && m_time == other.m_time;
}

// 比较运算符
bool DateTime::operator!=(const DateTime &other) const noexcept {
    return m_date != other.m_date || m_time != other.m_time;
}

bool DateTime::operator<(const DateTime &other) const noexcept {
    if (m_date != other.m_date) {
        return m_date < other.m_date;
    }
    return m_time < other.m_time;
}

bool DateTime::operator<=(const DateTime &other) const noexcept {
    return *this < other || *this == other;
}

bool DateTime::operator>(const DateTime &other) const noexcept {
    return !(*this <= other);
}

bool DateTime::operator>=(const DateTime &other) const noexcept {
    return !(*this < other);
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
    return toTimePoint() - other.toTimePoint();
}

// 辅助方法
std::optional<DateTime> DateTime::parseISO(std::string_view str) noexcept {
    // 匹配 YYYY-MM-DDTHH:MM:SS.sssZ 或 YYYY-MM-DD HH:MM:SS 格式，支持时区
    std::regex iso_regex(
        R"((\d{4})-(\d{1,2})-(\d{1,2})[T ](\d{1,2}):(\d{1,2}):(\d{1,2})(?:\.(\d{1,3}))?(?:(Z)|([+-])(\d{2}):(\d{2}))?$)");
    std::string s{str};
    std::smatch match;

    if (!std::regex_match(s, match, iso_regex)) {
        return std::nullopt;
    }

    try {
        int year = std::stoi(match[1].str());
        uint8_t month = std::stoi(match[2].str());
        uint8_t day = std::stoi(match[3].str());
        uint8_t hour = std::stoi(match[4].str());
        uint8_t minute = std::stoi(match[5].str());
        uint8_t second = std::stoi(match[6].str());
        uint16_t millisecond = match[7].matched ? std::stoi(match[7].str()) : 0;

        // 处理毫秒数的位数（如果只有1-2位，需要补齐到3位）
        if (match[7].matched) {
            std::string ms_str = match[7].str();
            if (ms_str.length() == 1) {
                millisecond *= 100;
            } else if (ms_str.length() == 2) {
                millisecond *= 10;
            }
        }

        if (year < 1900 || year > 3000 || month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 ||
            second > 59 || millisecond > 999) {
            return std::nullopt;
        }

        Date date{year, month, day};
        Time time{hour, minute, second, millisecond};

        if (!date.isValid() || !time.isValid()) {
            return std::nullopt;
        }

        DateTime result{date, time};

        // 处理时区信息
        if (match[8].matched) {
            // UTC时区 (Z)
            // 输入已经是UTC时间，无需转换
        } else if (match[9].matched && match[10].matched && match[11].matched) {
            // 有时区偏移 (+/-HH:MM)
            std::string sign = match[9].str();
            int tz_hours = std::stoi(match[10].str());
            int tz_minutes = std::stoi(match[11].str());

            if (tz_hours < 0 || tz_hours > 23 || tz_minutes < 0 || tz_minutes > 59) {
                return std::nullopt;
            }

            // 计算总的偏移分钟数
            int total_offset_minutes = tz_hours * 60 + tz_minutes;
            if (sign == "-") {
                total_offset_minutes = -total_offset_minutes;
            }

            // 将时间转换为UTC（减去时区偏移）
            result = result.plusMinutes(-total_offset_minutes);
        }
        // 如果没有时区信息，假设为本地时间

        return result;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<DateTime> DateTime::parseCustom(std::string_view str, [[maybe_unused]] std::string_view format) noexcept {
    // 简化实现，仅支持基本格式
    return parseISO(str);
}

/**
 * @brief 获取当前日期时间的UTC偏移
 *
 * @return std::chrono::minutes UTC偏移（分钟）
 */
std::chrono::minutes DateTime::getUTCOffset() noexcept {
    try {
#ifdef _WIN32
        TIME_ZONE_INFORMATION tz_info;
        DWORD result = GetTimeZoneInformation(&tz_info);
        // 计算UTC偏移（分钟）
        int offset = -tz_info.Bias;
        if (result == TIME_ZONE_ID_DAYLIGHT) {
            offset -= tz_info.DaylightBias;
        } else {
            offset -= tz_info.StandardBias;
        }
        return std::chrono::hours(offset / 60) + std::chrono::minutes(offset % 60);
#else
        [[deprecated("不支持的平台（暂未实现）")]]
        int a = 1,
            b = a;
#endif
    } catch (const std::exception &) {
        return std::chrono::minutes(0);
    }
}

#ifdef QT_CORE_LIB
// Qt 转换构造函数
DateTime::DateTime(const QDateTime &qdatetime) noexcept {
    if (qdatetime.isValid()) { // 如果 Qt 日期时间有效
        m_date = Date{qdatetime.date()}; // 从 Qt 日期转换为 my::Date
        m_time = Time{qdatetime.time()}; // 从 Qt 时间转换为 my::Time
    } else {
        m_date = Date{1970, 1, 1};
        m_time = Time{0, 0, 0, 0};
    }
}

// Qt 转换静态方法
DateTime DateTime::fromQDateTime(const QDateTime &qdatetime) noexcept {
    return DateTime{qdatetime};
}

// Qt 转换方法
QDateTime DateTime::toQDateTime() const noexcept {
    if (!isValid()) {
        return QDateTime{};
    }
    QDate qdate{year(), month(), day()};
    QTime qtime{hour(), minute(), second(), millisecond()};
    return QDateTime{qdate, qtime};
}
#endif

} // namespace my