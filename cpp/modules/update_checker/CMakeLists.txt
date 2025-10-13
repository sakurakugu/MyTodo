# ============================================================
# MyTodo 更新检查器模块构建配置
# 提供版本检查和更新通知功能
#
# 作者： sakurakugu
# 创建日期： 2025-10-07 19:33:00(UTC+8) 周二
# 修改日期： 2025-10-07 19:33:00(UTC+8) 周二
# ============================================================

# 创建更新检查器模块静态库
add_library(mytodo_update_checker STATIC
    # 更新检查器类
    version_checker.cpp
    version_checker.h
)

# 查找所需的 Qt 组件
find_package(Qt6 REQUIRED COMPONENTS 
    Core        # 核心功能（QObject, QString, QTimer, QUuid等）
    Network     # 网络功能（网络请求需要）
)

# 设置包含目录
target_include_directories(mytodo_update_checker PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                    # 当前目录
    ${CMAKE_SOURCE_DIR}/cpp/foundation             # 基础模块（Qdatabase.h, network_request.h等）
)

# 链接依赖库
target_link_libraries(mytodo_update_checker
    PUBLIC
        Qt6::Core
        Qt6::Network
        nlohmann_json::nlohmann_json    # JSON 库（用于解析 GitHub API 响应）
        mytodo_foundation               # 基础模块（QDatabase, NetworkRequest类等）
)