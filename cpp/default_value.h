#pragma once

#include <QSet>
#include <QString>

namespace DefaultValues {
    constexpr auto baseUrl = "https://api.example.com";
    constexpr auto todoApiEndpoint = "/todo/todo_api.php";
    constexpr auto userApiEndpoint = "/user/user_api.php";

    const QSet<QString> booleanKeys = {
        "setting/isDarkMode",      // 是否启用深色模式
        "setting/preventDragging", // 是否防止窗口拖动
        "setting/autoSync",        // 是否自动同步
        "log/logToFile",           // 是否记录到文件
        "log/logToConsole"         // 是否输出到控制台
    };

    constexpr auto logFileName = "MyTodo"; // 日志文件名（不包括后缀 ".log")

}
