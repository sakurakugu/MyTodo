# ============================================================
# 基础底座：配置 / 工具 / 数据库 / 网络 / QtDebug
# 负责基础功能的实现，包括配置管理、工具函数、数据库访问、网络请求和 Qt 调试输出
#
# 作者： sakurakugu
# 创建日期： 2025-10-05 02:42:00(UTC+8) 周日
# 修改日期： 2025-10-06 00:53:59(UTC+8) 周一
# ============================================================
add_library(mytodo_foundation STATIC
    config.cpp
    config.h
    default_value.h
    network_proxy.cpp
    network_proxy.h
    network_request.cpp
    network_request.h
    qt_debug.cpp
    qt_debug.h
    utility.cpp
    utility.h
)

# 查找 Qt 库
find_package(Qt6 REQUIRED COMPONENTS 
    Core Network Gui)

# 将包含目录添加到库中
target_include_directories(mytodo_foundation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}    # 允许其它模块用 #include "logger.h" 等    
    ${INCLUDE_DIR}                 # 允许包含生成的 version.h
)

# 链接 Qt 库和 toml++ 库
target_link_libraries(mytodo_foundation
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Gui
        tomlplusplus::tomlplusplus
        my_logger
)