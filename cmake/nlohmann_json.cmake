# 使用 CMake 获取并构建 nlohmann/json 库
# 示例:
#   set(NLOHMANN_JSON_VERSION "3.11.3" CACHE STRING "nlohmann/json 版本号")
#   include(cmake/nlohmann_json.cmake)
#   add_nlohmann_json()
#
# 注意事项：
# 1. 通过 FetchContent 从 GitHub 下载 nlohmann/json 库
# 2. 创建 nlohmann_json::nlohmann_json 目标供项目使用
# 3. 这是一个 header-only 库，无需编译

include_guard(GLOBAL) # 防止重复包含

include(FetchContent)

if(NOT DEFINED NLOHMANN_JSON_VERSION)
    set(NLOHMANN_JSON_VERSION "3.11.3" CACHE STRING "nlohmann/json 版本号")
endif()

function(add_nlohmann_json)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v${NLOHMANN_JSON_VERSION}
        GIT_SHALLOW TRUE
    )

    # 设置选项以避免构建测试和示例
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install OFF CACHE INTERNAL "")
    
    FetchContent_MakeAvailable(nlohmann_json) # 确保库已被获取并构建

    # 定义编译时宏，方便别的模块判断是否启用该库
    target_compile_definitions(nlohmann_json INTERFACE
        NLOHMANN_JSON_ENABLED
    )
endfunction()