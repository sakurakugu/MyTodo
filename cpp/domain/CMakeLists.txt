# ============================================================
# MyTodo 领域基础模块构建配置
# 提供数据存储和同步服务的基础抽象类
#
# 作者： sakurakugu
# 创建日期： 2025-10-06 00:37:00(UTC+8) 周一
# 修改日期： 2025-10-06 02:13:23(UTC+8) 周一
# ============================================================

# 创建领域静态库
add_library(mytodo_domain STATIC
    # 数据存储基类
    base_data_storage.cpp
    base_data_storage.h
    
    # 同步服务基类
    base_sync_server.cpp
    base_sync_server.h
)

# 查找所需的 Qt 组件
find_package(Qt6 REQUIRED COMPONENTS 
    Core        # 核心功能（QObject, QString, QDateTime等）
    Network     # 网络功能（同步服务器需要）
)

# 设置包含目录
target_include_directories(mytodo_domain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                    # 当前目录
    ${CMAKE_SOURCE_DIR}/cpp/foundation             # 基础模块（Qdatabase.h等）
    ${CMAKE_SOURCE_DIR}/cpp/app    # 应用层目录
)

# 链接依赖库
target_link_libraries(mytodo_domain
    PUBLIC
        Qt6::Core
        Qt6::Network
        mytodo_foundation    # 基础模块（Database类等）
        mytodo_user          # 用户模块
        my_logger             # 日志模块
)
