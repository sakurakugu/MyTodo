# ============================================================
# MyTodo 设置模块构建配置
# 提供应用程序配置管理功能
#
# 作者： sakurakugu
# 创建日期： 2025-10-06 02:05:00(UTC+8) 周一
# 修改日期： 2025-10-06 02:13:23(UTC+8) 周一
# ============================================================

# 创建设置模块静态库
add_library(mytodo_setting STATIC
    # 设置管理类
    setting.cpp
    setting.h
)

# 查找所需的 Qt 组件
find_package(Qt6 REQUIRED COMPONENTS 
    Core        # 核心功能（QObject, QString, QVariant等）
    Network     # 网络功能（代理设置需要）
)

# 设置包含目录
target_include_directories(mytodo_setting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                    # 当前目录
    ${CMAKE_SOURCE_DIR}/cpp/foundation             # 基础模块（logger.h, config.h等）
)

# 链接依赖库
target_link_libraries(mytodo_setting
    PUBLIC
        Qt6::Core
        Qt6::Network
        mytodo_foundation        # 基础模块（Logger, Config类等）
        mytodo_backup            # 备份模块（BackupManager类等）
        mytodo_database          # 数据库模块（Database类等）
)