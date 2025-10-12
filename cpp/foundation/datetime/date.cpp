// ============================================================
// Date 类实现：日期处理类
//
// 作者： sakurakugu
// 创建日期： 2025-10-12 16:30:00 (UTC+8)
// ============================================================

#include "date.h"
#include <sstream>
#include <iomanip>
#include <regex>
#include <stdexcept>

namespace my {

// 构造函数
Date::Date(const year_month_day& ymd) noexcept : m_ymd(ymd) {}

Date::Date(int year, unsigned month, unsigned day) noexcept 
    : m_ymd(std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}) {}

Date::Date(const sys_days& days) noexcept : m_ymd(days) {}

Date::Date(const std::string& dateStr) {
    auto ymd = parseISO(dateStr);
    if (!ymd || !ymd->ok()) {
        m_ymd = year_month_day{std::chrono::year{1900}, std::chrono::month{1}, std::chrono::day{1}};
    } else {
        m_ymd = *ymd;
    }
}

Date::Date(const Date& other) : m_ymd(other.m_ymd) {}

// 静态工厂方法
Date Date::today() noexcept {
    auto now = std::chrono::system_clock::now();
    auto days = std::chrono::floor<std::chrono::days>(now);
    return Date{days};
}

Date Date::fromString(std::string_view str) noexcept {
    return fromISOString(str);
}

Date Date::fromISOString(std::string_view str) noexcept {
    auto ymd = parseISO(str);
    if (!ymd) {
        return Date{};
    }
    
    if (!ymd->ok()) {
        return Date{};
    }
    
    return Date{*ymd};
}

// 访问器
int Date::year() const noexcept {
    return static_cast<int>(m_ymd.year());
}

unsigned Date::month() const noexcept {
    return static_cast<unsigned>(m_ymd.month());
}

unsigned Date::day() const noexcept {
    return static_cast<unsigned>(m_ymd.day());
}

std::chrono::weekday Date::weekday() const noexcept {
    return std::chrono::weekday{toSysDays()};
}

int Date::dayOfWeek() const {
    auto wd = weekday();
    return static_cast<int>(wd.c_encoding()); // 0=周日, 1=周一, ..., 6=周六
}

unsigned Date::dayOfYear() const noexcept {
    auto jan1 = std::chrono::year_month_day{m_ymd.year(), std::chrono::month{1}, std::chrono::day{1}};
    return static_cast<unsigned>((toSysDays() - std::chrono::sys_days{jan1}).count()) + 1;
}

// 验证
bool Date::isValid() const noexcept {
    return m_ymd.ok();
}

bool Date::isLeapYear() const noexcept {
    return m_ymd.year().is_leap();
}

// 日期操作
Date& Date::addDays(int days) noexcept {
    m_ymd = toSysDays() + std::chrono::days{days};
    return *this;
}

Date& Date::addMonths(int months) noexcept {
    m_ymd = m_ymd + std::chrono::months{months};
    return *this;
}

Date& Date::addYears(int years) noexcept {
    m_ymd = m_ymd + std::chrono::years{years};
    return *this;
}

Date Date::plusDays(int days) const noexcept {
    return Date{*this}.addDays(days);
}

Date Date::plusMonths(int months) const noexcept {
    return Date{*this}.addMonths(months);
}

Date Date::plusYears(int years) const noexcept {
    return Date{*this}.addYears(years);
}

// 日期差值
int Date::daysTo(const Date& other) const noexcept {
    return static_cast<int>((other.toSysDays() - toSysDays()).count());
}

int Date::monthsTo(const Date& other) const noexcept {
    auto months = (other.year() - year()) * 12 + (static_cast<int>(other.month()) - static_cast<int>(month()));
    
    // 如果目标日期的天数小于当前日期的天数，则减去一个月
    if (other.day() < day()) {
        months--;
    }
    
    return months;
}

int Date::yearsTo(const Date& other) const noexcept {
    auto years = other.year() - year();
    
    // 如果目标日期的月日小于当前日期的月日，则减去一年
    if (other.month() < month() || 
        (other.month() == month() && other.day() < day())) {
        years--;
    }
    
    return years;
}

// 格式化
std::string Date::toString(std::string_view format) const {
    if (format == "yyyy-MM-dd" || format.empty()) {
        return toISOString();
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
    
    return result;
}

std::string Date::toISOString() const {
    return std::format("{:04d}-{:02d}-{:02d}", year(), month(), day());
}

// 转换
Date::sys_days Date::toSysDays() const noexcept {
    return std::chrono::sys_days{m_ymd};
}

Date::year_month_day Date::toYearMonthDay() const noexcept {
    return m_ymd;
}

// 赋值操作符
Date& Date::operator=(const Date& other) {
    if (this != &other) {
        m_ymd = other.m_ymd;
    }
    return *this;
}

// 比较运算符
bool Date::operator!=(const Date& other) const {
    return m_ymd != other.m_ymd;
}

bool Date::operator<(const Date& other) const {
    return toSysDays() < other.toSysDays();
}

bool Date::operator<=(const Date& other) const {
    return toSysDays() <= other.toSysDays();
}

bool Date::operator>(const Date& other) const {
    return toSysDays() > other.toSysDays();
}

bool Date::operator>=(const Date& other) const {
    return toSysDays() >= other.toSysDays();
}

// 算术运算符
Date& Date::operator+=(const days& d) noexcept {
    return addDays(static_cast<int>(d.count()));
}

Date& Date::operator-=(const days& d) noexcept {
    return addDays(-static_cast<int>(d.count()));
}

Date& Date::operator+=(const months& m) noexcept {
    return addMonths(static_cast<int>(m.count()));
}

Date& Date::operator-=(const months& m) noexcept {
    return addMonths(-static_cast<int>(m.count()));
}

Date& Date::operator+=(const years& y) noexcept {
    return addYears(static_cast<int>(y.count()));
}

Date& Date::operator-=(const years& y) noexcept {
    return addYears(-static_cast<int>(y.count()));
}

Date Date::operator+(const days& d) const noexcept {
    return plusDays(static_cast<int>(d.count()));
}

Date Date::operator-(const days& d) const noexcept {
    return plusDays(-static_cast<int>(d.count()));
}

Date Date::operator+(const months& m) const noexcept {
    return plusMonths(static_cast<int>(m.count()));
}

Date Date::operator-(const months& m) const noexcept {
    return plusMonths(-static_cast<int>(m.count()));
}

Date Date::operator+(const years& y) const noexcept {
    return plusYears(static_cast<int>(y.count()));
}

Date Date::operator-(const years& y) const noexcept {
    return plusYears(-static_cast<int>(y.count()));
}

Date::days Date::operator-(const Date& other) const noexcept {
    return toSysDays() - other.toSysDays();
}

// 辅助方法
std::optional<Date::year_month_day> Date::parseISO(std::string_view str) noexcept {
    // 匹配 YYYY-MM-DD 格式
    std::regex iso_regex(R"((\d{4})-(\d{1,2})-(\d{1,2}))");
    std::string s{str};
    std::smatch match;
    
    if (!std::regex_match(s, match, iso_regex)) {
        return std::nullopt;
    }
    
    try {
        int year = std::stoi(match[1].str());
        unsigned month = std::stoul(match[2].str());
        unsigned day = std::stoul(match[3].str());
        
        if (year < 1900 || year > 3000 || month < 1 || month > 12 || day < 1 || day > 31) {
            return std::nullopt;
        }
        
        return std::chrono::year_month_day{
            std::chrono::year{year},
            std::chrono::month{month},
            std::chrono::day{day}
        };
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Date::year_month_day> Date::parseCustom(std::string_view str, std::string_view format) noexcept {
    // 简化实现，仅支持基本格式
    return parseISO(str);
}

} // namespace my