/**
 * @file default_value.h
 * @brief C++23 优化的默认值定义
 *
 * 该文件定义了应用程序的默认配置值和常量，使用 C++23 特性进行优化。
 *
 * @author Sakurakugu
 * @date 2025-08-21 23:33:39(UTC+8) 周四
 * @version 2025-08-22 01:20:36(UTC+8) 周五
 */
#pragma once

#include <array>
#include <string_view>

namespace DefaultValues {
    inline constexpr std::string_view baseUrl{"https://api.example.com"};
    inline constexpr std::string_view todoApiEndpoint{"/todo/todo_api.php"};
    inline constexpr std::string_view userAuthApiEndpoint{"/auth_api.php"};
    inline constexpr std::string_view categoriesApiEndpoint{"/todo/categories_api.php"};
    
    inline constexpr std::array<std::string_view, 5> booleanKeys{
        "setting/isDarkMode",      // 是否启用深色模式
        "setting/preventDragging", // 是否防止窗口拖动
        "setting/autoSync",        // 是否自动同步
        "log/logToFile",           // 是否记录到文件
        "log/logToConsole"         // 是否输出到控制台
    };
    
    inline constexpr std::string_view logFileName{"MyTodo"}; // 日志文件名（不包括后缀 ".log")
} // namespace DefaultValues
