/**
 * @file formatter.h
 * @brief 日期时间格式化工具类
 *
 * (自动生成的头部；请完善 @brief 及详细描述)
 * @author sakurakugu
 * @date 2025-01-17 16:30:00 (UTC+8)
 */

#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace my {

/**
 * @brief 通用日期时间格式化工具类
 *
 * 提供统一的格式化逻辑，支持自定义占位符替换
 */
class DateTimeFormatter {
  public:
    using ReplacementMap = std::unordered_map<std::string, std::function<std::string()>>;

    static std::string format(std::string_view pattern, const ReplacementMap &replacements);

    static ReplacementMap createDateReplacements(int32_t year, uint8_t month, uint8_t day);

    static ReplacementMap createTimeReplacements(uint8_t hour, uint8_t minute, uint8_t second, uint16_t millisecond);

    static ReplacementMap createDateTimeReplacements(int32_t year, uint8_t month, uint8_t day, uint8_t hour,
                                                     uint8_t minute, uint8_t second, uint16_t millisecond);

  private:
    static std::string replacePlaceholder(std::string str, const std::string &placeholder,
                                          const std::string &replacement);
};

} // namespace my