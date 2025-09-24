# 版本号解析模块
# 从 version.in 文件读取版本信息并设置相应的 CMake 变量
#
# 作者： sakurakugu
# 首次创建日期： 2025-09-09 21:39:19(UTC+8) 周二
# 最后更新日期： 2025-09-15 22:34:47(UTC+8) 周一

# 读取 version.in 文件
file(STRINGS "${CMAKE_SOURCE_DIR}/version.in" VERSION_CONTENT)

foreach(line ${VERSION_CONTENT})
    # 跳过注释行和空行
    if(NOT line MATCHES "^#" AND NOT line STREQUAL "")
        string(STRIP "${line}" line_stripped)
        # 使用更简单的字符串替换方法
        if(line_stripped MATCHES "^VERSION_MAJOR[ \t]+([0-9]+)")
            set(VERSION_MAJOR ${CMAKE_MATCH_1})
        elseif(line_stripped MATCHES "^VERSION_MINOR[ \t]+([0-9]+)")
            set(VERSION_MINOR ${CMAKE_MATCH_1})
        elseif(line_stripped MATCHES "^VERSION_PATCH[ \t]+([0-9]+)")
            set(VERSION_PATCH ${CMAKE_MATCH_1})
        elseif(line_stripped MATCHES "^DATABASE_VERSION[ \t]+([0-9]+)")
            set(DATABASE_VERSION ${CMAKE_MATCH_1})
        endif()
    endif()
endforeach()

# 确保所有版本变量都有值
if(NOT DEFINED VERSION_MAJOR OR VERSION_MAJOR STREQUAL "")
    set(VERSION_MAJOR 0)
endif()
if(NOT DEFINED VERSION_MINOR OR VERSION_MINOR STREQUAL "")
    set(VERSION_MINOR 0)
endif()
if(NOT DEFINED VERSION_PATCH OR VERSION_PATCH STREQUAL "")
    set(VERSION_PATCH 0)
endif()
if(NOT DEFINED DATABASE_VERSION OR DATABASE_VERSION STREQUAL "")
    set(DATABASE_VERSION 0)
endif()

# 变量已经在当前作用域中设置，include时会自动传递到父作用域

# 生成版本号头文件
configure_file(
    "${CMAKE_SOURCE_DIR}/cpp/script/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
    @ONLY
)

# 设置全局变量，供主 CMakeLists.txt 使用
set(INCLUDE_DIR ${CMAKE_BINARY_DIR}/generated)