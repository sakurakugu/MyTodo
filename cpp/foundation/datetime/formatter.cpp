/**
 * @file formatter.cpp
 * @brief 日期时间格式化工具实现
 *
 * (自动生成的头部；请完善 @brief 及详细描述)
 * @author sakurakugu
 * @date 2025-01-17 16:30:00 (UTC+8)
 */

#include "formatter.h"
#include <format>

namespace my {

/**
 * @brief 格式化字符串，替换占位符
 * @param pattern 格式模式字符串
 * @param replacements 占位符替换映射
 * @return 格式化后的字符串
 */
std::string DateTimeFormatter::format(std::string_view pattern, const ReplacementMap &replacements) {
    std::string result{pattern};

    for (const auto &[placeholder, replacementFunc] : replacements) {
        result = replacePlaceholder(result, placeholder, replacementFunc());
    }

    return result;
}

/**
 * @brief 创建标准的日期替换映射
 * @param year 年份
 * @param month 月份
 * @param day 日期
 * @return 日期替换映射
 */
DateTimeFormatter::ReplacementMap DateTimeFormatter::createDateReplacements(int32_t year, uint8_t month, uint8_t day) {
    return {{"{YYYY}", [year]() { return std::format("{:04d}", year); }},
            {"{MM}", [month]() { return std::format("{:02d}", month); }},
            {"{DD}", [day]() { return std::format("{:02d}", day); }},
            {"{M}", [month]() { return std::to_string(month); }},
            {"{D}", [day]() { return std::to_string(day); }}};
}

/**
 * @brief 创建标准的时间替换映射
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @param millisecond 毫秒
 * @return 时间替换映射
 */
DateTimeFormatter::ReplacementMap DateTimeFormatter::createTimeReplacements(uint8_t hour, uint8_t minute,
                                                                            uint8_t second, uint16_t millisecond) {
    return {{"{HH}", [hour]() { return std::format("{:02d}", hour); }},
            {"{mm}", [minute]() { return std::format("{:02d}", minute); }},
            {"{ss}", [second]() { return std::format("{:02d}", second); }},
            {"{SSS}", [millisecond]() { return std::format("{:03d}", millisecond); }},
            {"{H}", [hour]() { return std::to_string(hour); }},
            {"{m}", [minute]() { return std::to_string(minute); }},
            {"{s}", [second]() { return std::to_string(second); }},
            {"{S}", [millisecond]() { return std::to_string(millisecond); }},
            {"{hh}",
             [hour]() {
                 int h12 = hour % 12;
                 if (h12 == 0)
                     h12 = 12;
                 return std::format("{:02d}", h12);
             }},
            {"{h}",
             [hour]() {
                 int h12 = hour % 12;
                 if (h12 == 0)
                     h12 = 12;
                 return std::to_string(h12);
             }},
            {"{A}", [hour]() { return hour < 12 ? "AM" : "PM"; }},
            {"{a}", [hour]() { return hour < 12 ? "am" : "pm"; }}};
}

/**
 * @brief 创建标准的日期时间替换映射
 * @param year 年份
 * @param month 月份
 * @param day 日期
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 * @param millisecond 毫秒
 * @return 日期时间替换映射
 */
DateTimeFormatter::ReplacementMap DateTimeFormatter::createDateTimeReplacements(int32_t year, uint8_t month,
                                                                                uint8_t day, uint8_t hour,
                                                                                uint8_t minute, uint8_t second,
                                                                                uint16_t millisecond) {
    auto dateReplacements = createDateReplacements(year, month, day);
    auto timeReplacements = createTimeReplacements(hour, minute, second, millisecond);

    // 合并两个映射
    dateReplacements.insert(timeReplacements.begin(), timeReplacements.end());
    return dateReplacements;
}

/**
 * @brief 替换字符串中的占位符
 * @param str 原始字符串
 * @param placeholder 占位符
 * @param replacement 替换内容
 * @return 替换后的字符串
 */
std::string DateTimeFormatter::replacePlaceholder(std::string str, const std::string &placeholder,
                                                  const std::string &replacement) {
    size_t pos = 0;
    while ((pos = str.find(placeholder, pos)) != std::string::npos) {
        str.replace(pos, placeholder.length(), replacement);
        pos += replacement.length();
    }
    return str;
}

} // namespace my