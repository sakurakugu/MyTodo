/**
 * @file utility.h
 * @brief Utility类的头文件
 * 该文件定义了Utility类，提供通用的实用工具函数。
 * @author Sakurakugu
 * @date 2025-09-23 18:32:17(UTC+8) 周二
 * @change 2025-10-05 20:51:32(UTC+8) 周日
 */
#pragma once

// 定义DateType概念，限制模板参数为QDate或QDateTime
// template <typename T>
// concept DateType = std::same_as<T, QDate> || std::same_as<T, QDateTime>;

class Utility {
  public:
    Utility(const Utility &) = delete;
    Utility &operator=(const Utility &) = delete;
    Utility(Utility &&) = delete;
    Utility &operator=(Utility &&) = delete;

  private:
    Utility() = delete;
    ~Utility() noexcept = delete;

};
