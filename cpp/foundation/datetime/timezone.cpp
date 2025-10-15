/**
 * @file timezone.cpp
 * @brief 时区管理类的实现
 *
 * @author sakurakugu
 * @date 2025-01-27 (UTC+8)
 */

#include "timezone.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace my {

TimeZone::TimeZone()
    : m_cachedOffset(minutes(0)),                     // 默认值
      m_lastUpdateTime(system_clock::now()),          //
      m_cacheValidityDuration(minutes(30)) {          // 默认30分钟缓存
    // 构造时立即初始化缓存
    updateCache();
}

/**
 * @brief 获取当前的UTC偏移量
 * @return std::chrono::minutes UTC偏移量（分钟）
 */
TimeZone::minutes TimeZone::getUTCOffset() noexcept {
    // 检查是否需要更新缓存
    if (needsUpdate()) {
        updateCache();
    }

    // 返回缓存的偏移量
    return m_cachedOffset.load();
}

/**
 * @brief 强制刷新时区信息
 *
 * 立即从系统重新获取时区信息，更新缓存
 */
void TimeZone::refreshTimeZone() noexcept {
    updateCache();
}

/**
 * @brief 设置缓存有效期
 * @param duration 缓存有效期（默认30分钟）
 */
void TimeZone::setCacheValidityDuration(std::chrono::minutes duration) noexcept {
    m_cacheValidityDuration.store(duration);
}

/**
 * @brief 检查缓存是否有效
 * @return bool 缓存是否仍然有效
 */
bool TimeZone::isCacheValid() const noexcept {
    return !needsUpdate();
}

/**
 * @brief 获取上次更新时间
 * @return time_point 上次更新的时间点
 */
TimeZone::time_point TimeZone::getLastUpdateTime() const noexcept {
    return m_lastUpdateTime.load();
}

/**
 * @brief 获取缓存的时区偏移量（不检查有效性）
 * @return minutes 缓存的偏移量
 */
TimeZone::minutes TimeZone::getCachedOffset() const noexcept {
    return m_cachedOffset.load();
}

/**
 * @brief 获取系统当前的UTC偏移量
 * @return std::chrono::minutes UTC偏移量（分钟）
 */
TimeZone::minutes TimeZone::getSystemUTCOffset() const noexcept {
    try {
#ifdef _WIN32
        TIME_ZONE_INFORMATION tz_info;
        DWORD result = GetTimeZoneInformation(&tz_info);

        // 计算UTC偏移（分钟）
        int offset = -tz_info.Bias;
        if (result == TIME_ZONE_ID_DAYLIGHT) {
            offset -= tz_info.DaylightBias;
        } else if (result == TIME_ZONE_ID_STANDARD) {
            offset -= tz_info.StandardBias;
        }

        return minutes(offset);
#else
        // 使用C++20的时区库
        try {
            [[deprecated("不支持的平台（暂未实现）")]]
            auto now = system_clock::now();
            auto local_time = std::chrono::current_zone()->to_local(now);
            auto utc_time = std::chrono::clock_cast<system_clock>(now);
            auto offset = std::chrono::duration_cast<minutes>(local_time - utc_time);
            return offset;
        } catch (...) {
            // 如果C++20时区库不可用，返回0偏移
            return minutes(0);
        }
#endif
    } catch (const std::exception &) {
        return minutes(0);
    }
}

/**
 * @brief 更新缓存的时区信息
 */
void TimeZone::updateCache() noexcept {
    try {
        auto newOffset = getSystemUTCOffset();
        m_cachedOffset.store(newOffset);
        m_lastUpdateTime.store(system_clock::now());
    } catch (...) {
        // 如果获取失败，保持之前的缓存值
        // 只更新时间戳，避免频繁重试
        m_lastUpdateTime.store(system_clock::now());
    }
}

/**
 * @brief 检查是否需要更新缓存
 * @return bool 是否需要更新
 */
bool TimeZone::needsUpdate() const noexcept {
    auto now = system_clock::now();
    auto lastUpdate = m_lastUpdateTime.load();
    auto validityDuration = m_cacheValidityDuration.load();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - lastUpdate);
    return elapsed >= validityDuration;
}

} // namespace my