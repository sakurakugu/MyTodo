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

constexpr const char *appName{"MyTodo"}; // 应用程序名称

constexpr const char *baseUrl{"https://api.example.com"};            // 基础 API URL
constexpr const char *todoApiEndpoint{"/todo/todo_api"};             // 待办事项 API 端点
constexpr const char *userAuthApiEndpoint{"/auth_api"};              // 用户认证 API 端点
constexpr const char *categoriesApiEndpoint{"/todo/categories_api"}; // 分类 API 端点

// constexpr const int 令牌刷新间隔{3600}; // (秒)
// constexpr const int auto_sync_interval{30};        // 自动同步间隔（分钟）

} // namespace DefaultValues
