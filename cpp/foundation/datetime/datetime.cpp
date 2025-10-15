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
#include <regex>

#ifdef QT_CORE_LIB
#include <QDateTime>
#endif

namespace my {

// 构造函数
DateTime::DateTime() noexcept
    : m_date(Date::today()), m_time(Time::now()), m_tzOffset(TimeZone::GetInstance().getUTCOffset()) {}

DateTime::DateTime(const int64_t timestamp, std::optional<minutes> tzOffset) noexcept
    : DateTime(system_clock::from_time_t(timestamp), tzOffset.value_or(my::TimeZone::GetInstance().getUTCOffset())) {}

DateTime::DateTime(const time_point &tp, std::optional<minutes> tzOffset) noexcept {
    auto days = std::chrono::floor<std::chrono::days>(tp);
    auto time_since_midnight = tp - days;

    auto h = std::chrono::floor<std::chrono::hours>(time_since_midnight);
    auto m = std::chrono::floor<std::chrono::minutes>(time_since_midnight - h);
    auto s = std::chrono::floor<std::chrono::seconds>(time_since_midnight - h - m);
    auto ms = std::chrono::floor<std::chrono::milliseconds>(time_since_midnight - h - m - s);

    m_date = Date{std::chrono::sys_days{days}};
    m_time = Time{static_cast<uint8_t>(h.count()), static_cast<uint8_t>(m.count()), static_cast<uint8_t>(s.count()),
                  static_cast<uint16_t>(ms.count())};
    m_tzOffset = tzOffset.value_or(my::TimeZone::GetInstance().getUTCOffset());
}

DateTime::DateTime(const Date &date, const Time &time, std::optional<minutes> tzOffset) noexcept
    : m_date(date), m_time(time), m_tzOffset(tzOffset.value_or(TimeZone::GetInstance().getUTCOffset())) {}
DateTime::DateTime(int32_t year, uint8_t month, uint8_t day, const Time &time, std::optional<minutes> tzOffset) noexcept
    : m_date(year, month, day), m_time(time), m_tzOffset(tzOffset.value_or(TimeZone::GetInstance().getUTCOffset())) {}

DateTime::DateTime(const Date &date, uint8_t hour, uint8_t minute, uint8_t second, uint16_t millisecond,
                   std::optional<minutes> tzOffset) noexcept
    : m_date(date), m_time(hour, minute, second, millisecond),
      m_tzOffset(tzOffset.value_or(TimeZone::GetInstance().getUTCOffset())) {}

DateTime::DateTime(int32_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second,
                   uint16_t millisecond, std::optional<minutes> tzOffset) noexcept
    : m_date(year, month, day), m_time(hour, minute, second, millisecond),
      m_tzOffset(tzOffset.value_or(TimeZone::GetInstance().getUTCOffset())) {}

DateTime::DateTime(const std::string &dateTimeStr) {
    auto dt = parseISO(dateTimeStr);
    if (!dt) {
        m_date = Date{};
        m_time = Time{};
        m_tzOffset = std::chrono::minutes{0};
    } else {
        m_date = dt->m_date;
        m_time = dt->m_time;
        m_tzOffset = dt->m_tzOffset;
    }
}

#ifdef NLOHMANN_JSON_ENABLED
// 序列化
void to_json(nlohmann::json &j, const DateTime &dt) {
    if (dt.isValid()) {
        j = dt.toISOString();
    } else {
        j = nullptr;
    }
}

// 反序列化
void from_json(const nlohmann::json &j, DateTime &dt) {
    if (j.is_string()) {
        dt = DateTime{j.get<std::string>()};
    } else {
        dt = DateTime{};
    }
}
#endif

// 静态工厂方法
/**
 * @brief 获取当前日期时间
 *
 * @param tz 时区，默认本地时区
 * @return DateTime 当前日期时间
 */
DateTime DateTime::now(TimeZoneType tz) noexcept {
    auto now = system_clock::now();
    if (tz == TimeZoneType::UTC) {
        return DateTime{now, minutes{0}};
    } else {
        auto offset = my::TimeZone::GetInstance().getUTCOffset();
        return DateTime{now, offset};
    }
}

DateTime DateTime::utcNow() noexcept {
    return now(TimeZoneType::UTC);
}

DateTime DateTime::today(TimeZoneType tz) noexcept {
    auto now = DateTime::now(tz);
    return DateTime{now.year(), now.month(), now.day(), 0, 0, 0, 0, now.m_tzOffset};
}

DateTime DateTime::fromString(std::string_view str) noexcept {
    return fromISOString(str);
}

DateTime DateTime::fromISOString(std::string_view str) noexcept {
    auto tp = parseISO(str);
    if (!tp) {
        // 返回一个无效的DateTime
        DateTime invalid;
        invalid.m_date = Date{}; // 默认构造的Date是无效的
        invalid.m_time = Time{};
        invalid.m_tzOffset = minutes{0};
        return invalid;
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

std::chrono::minutes DateTime::tzOffset() const noexcept {
    return m_tzOffset;
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
    // 记录原始时间的毫秒数
    int64_t originalMs = m_time.toMilliseconds();

    // 计算新的总毫秒数
    int64_t newTotalMs = originalMs + ms;

    // 计算需要调整的天数
    int64_t dayAdjustment = 0;
    if (newTotalMs >= 86400000) {
        dayAdjustment = newTotalMs / 86400000;
        newTotalMs %= 86400000;
    } else if (newTotalMs < 0) {
        // 计算需要减去的天数（向上取整的绝对值）
        dayAdjustment = -((-newTotalMs - 1) / 86400000 + 1);
        newTotalMs = newTotalMs - dayAdjustment * 86400000;
    }

    // 调整日期
    if (dayAdjustment != 0) {
        m_date.addDays(static_cast<int32_t>(dayAdjustment));
    }

    // 设置新的时间（从毫秒数创建）
    m_time = Time::fromMilliseconds(newTotalMs);

    return *this;
}

DateTime &DateTime::addSeconds(int64_t seconds) noexcept {
    // 记录原始时间的毫秒数
    int64_t originalMs = m_time.toMilliseconds();

    // 添加秒数
    m_time.addSeconds(seconds);

    // 计算实际添加的毫秒数
    int64_t addedMs = seconds * 1000;

    // 计算新的总毫秒数
    int64_t newTotalMs = originalMs + addedMs;

    // 计算需要调整的天数
    int64_t dayAdjustment = 0;
    if (newTotalMs >= 86400000) {
        dayAdjustment = newTotalMs / 86400000;
    } else if (newTotalMs < 0) {
        dayAdjustment = (newTotalMs - 86399999) / 86400000; // 向下取整
    }

    // 调整日期
    if (dayAdjustment != 0) {
        m_date.addDays(static_cast<int32_t>(dayAdjustment));
    }

    return *this;
}

DateTime &DateTime::addMinutes(int64_t minutes) noexcept {
    // 记录原始时间的毫秒数
    int64_t originalMs = m_time.toMilliseconds();

    // 添加分钟数
    m_time.addMinutes(minutes);

    // 计算实际添加的毫秒数
    int64_t addedMs = minutes * 60000;

    // 计算新的总毫秒数
    int64_t newTotalMs = originalMs + addedMs;

    // 计算需要调整的天数
    int64_t dayAdjustment = 0;
    if (newTotalMs >= 86400000) {
        dayAdjustment = newTotalMs / 86400000;
    } else if (newTotalMs < 0) {
        dayAdjustment = (newTotalMs - 86399999) / 86400000; // 向下取整
    }

    // 调整日期
    if (dayAdjustment != 0) {
        m_date.addDays(static_cast<int32_t>(dayAdjustment));
    }

    return *this;
}

DateTime &DateTime::addHours(int64_t hours) noexcept {
    // 记录原始时间的毫秒数
    int64_t originalMs = m_time.toMilliseconds();

    // 添加小时数
    m_time.addHours(hours);

    // 计算实际添加的毫秒数
    int64_t addedMs = hours * 3600000;

    // 计算新的总毫秒数
    int64_t newTotalMs = originalMs + addedMs;

    // 计算需要调整的天数
    int64_t dayAdjustment = 0;
    if (newTotalMs >= 86400000) {
        dayAdjustment = newTotalMs / 86400000;
    } else if (newTotalMs < 0) {
        dayAdjustment = (newTotalMs - 86399999) / 86400000; // 向下取整
    }

    // 调整日期
    if (dayAdjustment != 0) {
        m_date.addDays(static_cast<int32_t>(dayAdjustment));
    }

    return *this;
}

DateTime &DateTime::addDays(int32_t days) noexcept {
    m_date.addDays(days);
    return *this;
}

DateTime &DateTime::addMonths(int32_t months) noexcept {
    m_date.addMonths(months);
    return *this;
}

DateTime &DateTime::addYears(int32_t years) noexcept {
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

DateTime DateTime::plusDays(int32_t days) const noexcept {
    return DateTime{*this}.addDays(days);
}

DateTime DateTime::plusMonths(int32_t months) const noexcept {
    return DateTime{*this}.addMonths(months);
}

DateTime DateTime::plusYears(int32_t years) const noexcept {
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

std::string DateTime::toLocalString() const {
    DateTime dt = DateTime{*this} + m_tzOffset;
    return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}", dt.year(), dt.month(), dt.day(), dt.hour(),
                       dt.minute(), dt.second());
}

std::string DateTime::toISOString(TimeZoneType tz) const {
    if (tz == TimeZoneType::Local && m_tzOffset != minutes(0)) {
        DateTime dt = DateTime{*this} + m_tzOffset;
        int32_t offsetHours = std::chrono::duration_cast<hours>(m_tzOffset).count();
        int32_t offsetMinutes = std::chrono::duration_cast<minutes>(m_tzOffset).count() % 60;
        return std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.{:03d}{}{:02d}:{:02d}", dt.year(), dt.month(),
                           dt.day(), dt.hour(), dt.minute(), dt.second(), dt.millisecond(),
                           (offsetHours > 0 ? "+" : "-"), std::abs(offsetHours), std::abs(offsetMinutes));
    } else {
        return std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.{:03d}Z", year(), month(), day(), hour(),
                           minute(), second(), millisecond());
    }
}

std::string DateTime::toDateString() const {
    return date().toISOString();
}

std::string DateTime::toTimeString() const {
    return time().toString(); // 不含毫秒
}

// 转换
std::chrono::system_clock::time_point DateTime::toTimePoint() const noexcept {
    auto days = m_date.toSysDays();
    auto time_duration = m_time.toDuration();
    auto local_time_point = days + time_duration;

    // 如果有时区偏移，需要减去偏移得到UTC时间
    // 因为存储的是本地时间，要转换为UTC需要减去偏移
    return local_time_point - m_tzOffset;
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
        // 如果当前时间已经有时区偏移信息，使用它进行转换
        if (m_tzOffset != minutes{0}) {
            // 减去时区偏移得到UTC时间
            auto result = *this;
            result.addMinutes(-m_tzOffset.count());
            result.m_tzOffset = minutes{0};
            return result;
        }

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
        // 如果转换失败，使用简单的偏移计算
        auto offset = my::TimeZone::GetInstance().getUTCOffset();
        auto result = *this;
        result.addMinutes(-offset.count());
        result.m_tzOffset = minutes{0};
        return result;
    }
}

DateTime DateTime::toLocal() const noexcept {
    try {
        // 如果当前时间已经有时区偏移信息，检查是否需要转换
        auto localOffset = my::TimeZone::GetInstance().getUTCOffset();
        if (m_tzOffset == localOffset) {
            // 已经是本地时间
            return *this;
        }

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

        return DateTime{localDate, localTime, localOffset};
    } catch (...) {
        // 如果转换失败，使用简单的偏移计算
        auto offset = my::TimeZone::GetInstance().getUTCOffset();
        auto result = *this;
        result.addMinutes(offset.count());
        result.m_tzOffset = offset;
        return result;
    }
}

bool DateTime::operator==(const DateTime &other) const noexcept {
    // 储存的是UTC时间，直接比较，时间是单独储存的，不需要考虑时区偏移
    return m_date == other.m_date && m_time == other.m_time;
}

// 比较运算符
bool DateTime::operator!=(const DateTime &other) const noexcept {
    return !(*this == other);
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
        Time time(hour, minute, second, millisecond);

        if (!date.isValid() || !time.isValid()) {
            return std::nullopt;
        }

        // 处理时区信息
        int tzOffset = 0;
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
            int tzOffset = tz_hours * 60 + tz_minutes;
            if (sign == "-") {
                tzOffset = -tzOffset;
            }
        }
        return DateTime{date, time, minutes(tzOffset)};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<DateTime> DateTime::parseCustom(std::string_view str, [[maybe_unused]] std::string_view format) noexcept {
    // 简化实现，仅支持基本格式
    return parseISO(str);
}

#ifdef QT_CORE_LIB
// Qt 转换构造函数
DateTime::DateTime(const QDateTime &qdatetime) noexcept {
    if (qdatetime.isValid()) {           // 如果 Qt 日期时间有效
        m_date = Date{qdatetime.date()}; // 从 Qt 日期转换为 my::Date
        m_time = Time{qdatetime.time()}; // 从 Qt 时间转换为 my::Time
    } else {
        m_date = Date{1970, 1, 1};
        m_time = Time{0, 0, 0, 0};
        m_tzOffset = std::chrono::minutes{0};
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