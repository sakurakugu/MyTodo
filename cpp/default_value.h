/**
 * @file default_value.h
 * @brief C++23 优化的默认值定义
 *
 * 该文件定义了应用程序的默认配置值和常量，使用 C++23 特性进行优化。
 *
 * @author Sakurakugu
 * @date 2025-08-21 23:33:39(UTC+8) 周四
 * @change 2025-08-31 15:07:38(UTC+8) 周日
 * @version 0.4.0
 */
#pragma once

#include <string_view>

namespace DefaultValues {
    inline constexpr std::string_view baseUrl{"https://api.example.com"};
    inline constexpr std::string_view todoApiEndpoint{"/todo/todo_api.php"};
    inline constexpr std::string_view userAuthApiEndpoint{"/auth_api.php"};
    inline constexpr std::string_view categoriesApiEndpoint{"/todo/categories_api.php"};

    inline constexpr std::string_view appName{"MyTodo"}; // 应用程序名称
} // namespace DefaultValues
