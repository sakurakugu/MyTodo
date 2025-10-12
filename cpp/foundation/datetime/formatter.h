// ============================================================
// DateTimeFormatter 类：通用日期时间格式化工具
// 提供统一的格式化逻辑，减少代码重复
//
// 作者： sakurakugu
// 创建日期： 2025-01-17 (UTC+8)
// ============================================================

#pragma once

#include <string>
#include <string_view>
#include <format>
#include <unordered_map>
#include <functional>

namespace my {

/**
 * @brief 通用日期时间格式化工具类
 * 
 * 提供统一的格式化逻辑，支持自定义占位符替换
 */
class DateTimeFormatter {
public:
    using ReplacementMap = std::unordered_map<std::string, std::function<std::string()>>;
    
    /**
     * @brief 格式化字符串，替换占位符
     * @param pattern 格式模式字符串
     * @param replacements 占位符替换映射
     * @return 格式化后的字符串
     */
    static std::string format(std::string_view pattern, const ReplacementMap& replacements);
    
    /**
     * @brief 创建标准的日期替换映射
     * @param year 年份
     * @param month 月份
     * @param day 日期
     * @return 日期替换映射
     */
    static ReplacementMap createDateReplacements(int year, unsigned month, unsigned day);
    
    /**
     * @brief 创建标准的时间替换映射
     * @param hour 小时
     * @param minute 分钟
     * @param second 秒
     * @param millisecond 毫秒
     * @return 时间替换映射
     */
    static ReplacementMap createTimeReplacements(int hour, int minute, int second, int millisecond);
    
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
    static ReplacementMap createDateTimeReplacements(int year, unsigned month, unsigned day,
                                                   int hour, int minute, int second, int millisecond);

private:
    /**
     * @brief 替换字符串中的占位符
     * @param str 原始字符串
     * @param placeholder 占位符
     * @param replacement 替换内容
     * @return 替换后的字符串
     */
    static std::string replacePlaceholder(std::string str, const std::string& placeholder, 
                                        const std::string& replacement);
};

} // namespace my