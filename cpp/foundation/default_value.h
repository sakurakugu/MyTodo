/**
 * @file default_value.h
 * @brief C++23 优化的默认值定义
 *
 * 该文件定义了应用程序的默认配置值和常量，使用 C++23 特性进行优化。
 *
 * @author Sakurakugu
 * @date 2025-08-21 23:33:39(UTC+8) 周四
 * @change 2025-09-22 16:33:30(UTC+8) 周一
 */
#pragma once

namespace DefaultValues {

constexpr const char *baseUrl{"https://api.example.com"};   // 基础 API URL（这里不改，在配置文件改）
constexpr const char *apiVersion{"v1"};                     // API 版本
constexpr const char *todoApiEndpoint{"/todos"};            // 待办事项 API 端点
constexpr const char *userAuthApiEndpoint{"/auth"};         // 用户认证 API 端点
constexpr const char *categoriesApiEndpoint{"/categories"}; // 分类 API 端点

} // namespace DefaultValues
