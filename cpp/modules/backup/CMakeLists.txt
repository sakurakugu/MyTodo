# ============================================================
# MyTodo 备份模块构建配置
# 提供应用程序备份管理功能
#
# 作者： sakurakugu
# 创建日期： 2025-10-08 23:54:00(UTC+8) 周三
# 修改日期： 2025-10-08 23:54:00(UTC+8) 周三
# ============================================================

# 创建设置模块静态库
add_library(mytodo_backup STATIC
    # 备份管理类
    backup_manager.cpp
    backup_manager.h
)

# 查找所需的 Qt 组件
find_package(Qt6 REQUIRED COMPONENTS 
    Core        # 核心功能（QObject, QString, QVariant等）
)

# 设置包含目录
target_include_directories(mytodo_backup PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                    # 当前目录
    ${CMAKE_SOURCE_DIR}/cpp/foundation             # 基础模块（logger.h, config.h等）
)

# 链接依赖库
target_link_libraries(mytodo_backup
    PUBLIC
        Qt6::Core
        mytodo_foundation        # 基础模块（Logger, Config类等）
)