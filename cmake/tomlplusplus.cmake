# 使用 CMake 获取并构建 toml++ 库
# 示例:
#   set(TOMLPLUSPLUS_VERSION "3.4.0" CACHE STRING "tomlplusplus 版本号")
#   include(cmake/tomlplusplus.cmake)
#   add_tomlplusplus()
#
# 注意事项：
# 1. 通过 FetchContent 从 GitHub 下载 tomlplusplus 库
# 2. 创建 tomlplusplus::tomlplusplus 目标供项目使用
# 3. 这是一个 header-only 库，无需编译

include_guard(GLOBAL) # 防止重复包含

include(FetchContent)

if(NOT DEFINED TOMLPLUSPLUS_VERSION)
    set(TOMLPLUSPLUS_VERSION "3.4.0" CACHE STRING "tomlplusplus 版本号")
endif()

function(add_tomlplusplus)
    FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG        v${TOMLPLUSPLUS_VERSION}
    )

    FetchContent_MakeAvailable(tomlplusplus)
endfunction()
