// ============================================================
// DateTimeFormatter 类实现：通用日期时间格式化工具
// 提供统一的格式化逻辑，减少代码重复
//
// 作者： sakurakugu
// 创建日期： 2025-01-17 (UTC+8)
// ============================================================

#include "formatter.h"
#include <format>

namespace my {

std::string DateTimeFormatter::format(std::string_view pattern, const ReplacementMap& replacements) {
    std::string result{pattern};
    
    for (const auto& [placeholder, replacementFunc] : replacements) {
        result = replacePlaceholder(result, placeholder, replacementFunc());
    }
    
    return result;
}

DateTimeFormatter::ReplacementMap DateTimeFormatter::createDateReplacements(int year, unsigned month, unsigned day) {
    return {
        {"{YYYY}", [year]() { return std::format("{:04d}", year); }},
        {"{MM}", [month]() { return std::format("{:02d}", month); }},
        {"{DD}", [day]() { return std::format("{:02d}", day); }},
        {"{M}", [month]() { return std::to_string(month); }},
        {"{D}", [day]() { return std::to_string(day); }}
    };
}

DateTimeFormatter::ReplacementMap DateTimeFormatter::createTimeReplacements(int hour, int minute, int second, int millisecond) {
    return {
        {"{HH}", [hour]() { return std::format("{:02d}", hour); }},
        {"{mm}", [minute]() { return std::format("{:02d}", minute); }},
        {"{ss}", [second]() { return std::format("{:02d}", second); }},
        {"{SSS}", [millisecond]() { return std::format("{:03d}", millisecond); }},
        {"{H}", [hour]() { return std::to_string(hour); }},
        {"{m}", [minute]() { return std::to_string(minute); }},
        {"{s}", [second]() { return std::to_string(second); }},
        {"{S}", [millisecond]() { return std::to_string(millisecond); }},
        {"{hh}", [hour]() { 
            int h12 = hour % 12;
            if (h12 == 0) h12 = 12;
            return std::format("{:02d}", h12);
        }},
        {"{h}", [hour]() { 
            int h12 = hour % 12;
            if (h12 == 0) h12 = 12;
            return std::to_string(h12);
        }},
        {"{A}", [hour]() { return hour < 12 ? "AM" : "PM"; }},
        {"{a}", [hour]() { return hour < 12 ? "am" : "pm"; }}
    };
}

DateTimeFormatter::ReplacementMap DateTimeFormatter::createDateTimeReplacements(int year, unsigned month, unsigned day,
                                                                               int hour, int minute, int second, int millisecond) {
    auto dateReplacements = createDateReplacements(year, month, day);
    auto timeReplacements = createTimeReplacements(hour, minute, second, millisecond);
    
    // 合并两个映射
    dateReplacements.insert(timeReplacements.begin(), timeReplacements.end());
    return dateReplacements;
}

std::string DateTimeFormatter::replacePlaceholder(std::string str, const std::string& placeholder, 
                                                 const std::string& replacement) {
    size_t pos = 0;
    while ((pos = str.find(placeholder, pos)) != std::string::npos) {
        str.replace(pos, placeholder.length(), replacement);
        pos += replacement.length();
    }
    return str;
}

} // namespace my