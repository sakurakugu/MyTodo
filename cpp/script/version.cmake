# 版本号解析模块
# 从 version.in 文件读取版本信息并设置相应的 CMake 变量

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
        endif()
    endif()
endforeach()

# 确保所有版本变量都有值
if(NOT DEFINED VERSION_MAJOR OR VERSION_MAJOR STREQUAL "")
    set(VERSION_MAJOR 0)
endif()
if(NOT DEFINED VERSION_MINOR OR VERSION_MINOR STREQUAL "")
    set(VERSION_MINOR 4)
endif()
if(NOT DEFINED VERSION_PATCH OR VERSION_PATCH STREQUAL "")
    set(VERSION_PATCH 0)
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