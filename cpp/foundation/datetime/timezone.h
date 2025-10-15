/**
 * @file timezone.h
 * @brief 时区管理类 - 提供时区信息缓存和管理功能
 *
 * 该类负责管理系统时区信息，避免频繁的系统调用，提供缓存机制
 * @author sakurakugu
 * @date 2025-01-27 (UTC+8)
 */

#pragma once

#include <atomic>
#include <chrono>

namespace my {

// 时区类型
enum class TimeZoneType {
    UTC,
    Local
};

/**
 * @brief TimeZone 类：时区信息管理器
 *
 * 该类提供了时区信息的缓存和管理功能，包括：
 * - 时区偏移的缓存机制
 * - 定期更新时区信息
 * - 线程安全的时区信息访问
 * - 夏令时变化的自动检测
 */
class TimeZone {
  public:
    using minutes = std::chrono::minutes;
    using system_clock = std::chrono::system_clock;
    using time_point = std::chrono::system_clock::time_point;

    static TimeZone &GetInstance() {
        static TimeZone instance;
        return instance;
    }
    // 删除拷贝和移动操作 - 单例模式
    TimeZone(const TimeZone &) = delete;
    TimeZone &operator=(const TimeZone &) = delete;
    TimeZone(TimeZone &&) = delete;
    TimeZone &operator=(TimeZone &&) = delete;

    minutes getUTCOffset() noexcept;                                       // 获取当前的UTC偏移量
    void refreshTimeZone() noexcept;                                       // 强制刷新时区信息
    void setCacheValidityDuration(std::chrono::minutes duration) noexcept; // 设置缓存有效期
    bool isCacheValid() const noexcept;                                    // 检查缓存是否有效
    time_point getLastUpdateTime() const noexcept;                         // 获取上次更新时间
    minutes getCachedOffset() const noexcept;                              // 获取缓存的时区偏移量（不检查有效性）

  private:
    TimeZone();
    minutes getSystemUTCOffset() const noexcept; // 从系统获取UTC偏移量
    void updateCache() noexcept;                 // 更新缓存的时区信息
    bool needsUpdate() const noexcept;           // 检查是否需要更新缓存

  private:
    std::atomic<minutes> m_cachedOffset;          ///< 缓存的UTC偏移量
    std::atomic<time_point> m_lastUpdateTime;     ///< 上次更新时间
    std::atomic<minutes> m_cacheValidityDuration; ///< 缓存有效期（默认30分钟）
};

} // namespace my