# ============================================================
# MyTodo 用户认证模块构建配置
# 提供用户认证和授权管理功能
#
# 作者： sakurakugu
# 创建日期： 2025-10-06 02:05:00(UTC+8) 周一
# 修改日期： 2025-10-06 02:13:23(UTC+8) 周一
# ============================================================

# 创建用户认证模块静态库
add_library(mytodo_user STATIC
    # 用户认证类
    user_auth.cpp
    user_auth.h
)

# 查找所需的 Qt 组件
find_package(Qt6 REQUIRED COMPONENTS 
    Core        # 核心功能（QObject, QString, QTimer, QUuid等）
    Network     # 网络功能（网络请求需要）
    Sql         # SQL数据库支持（用户数据持久化）
)

# 设置包含目录
target_include_directories(mytodo_user PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                    # 当前目录
    ${CMAKE_SOURCE_DIR}/cpp/foundation             # 基础模块（database.h, network_request.h等）
)

# 链接依赖库
target_link_libraries(mytodo_user
    PUBLIC
        Qt6::Core
        Qt6::Network
        mytodo_foundation    # 基础模块（Database, NetworkRequest类等）
        mytodo_setting       # 设置模块
)