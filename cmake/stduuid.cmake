# 使用 CMake 获取并构建 stduuid 库
# 示例:
#   include(cmake/stduuid.cmake)
#   add_stduuid()
#
# 注意事项：
# 1. 通过 FetchContent 从 GitHub 下载 stduuid 库
# 2. 创建 stduuid::stduuid 目标供项目使用
# 3. 这是一个 header-only 库，无需编译

include_guard(GLOBAL) # 防止重复包含

include(FetchContent)

if(NOT DEFINED STDUUID_VERSION)
    set(STDUUID_VERSION "1.2.3" CACHE STRING "stduuid 版本号")
endif()

function(add_stduuid)
    FetchContent_Declare(
        stduuid
        GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
        GIT_TAG v${STDUUID_VERSION}
        GIT_SHALLOW TRUE
    )
    
    FetchContent_MakeAvailable(stduuid) # 确保库已被获取并构建

    # 设置选项以避免构建测试和示例
    set(UUID_BUILD_TESTS OFF CACHE INTERNAL "")
    
    # 创建接口库目标
    if(NOT TARGET stduuid::stduuid)
        add_library(stduuid::stduuid INTERFACE IMPORTED)
        target_include_directories(stduuid::stduuid INTERFACE 
            ${stduuid_SOURCE_DIR}/include
        )
        
        # 启用系统生成器
        target_compile_definitions(stduuid::stduuid INTERFACE UUID_SYSTEM_GENERATOR)
    endif()
endfunction()