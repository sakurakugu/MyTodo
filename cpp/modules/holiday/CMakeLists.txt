 # ============================================================
# MyTodo 节假日管理模块构建配置
# 负责节假日数据的获取、存储和查询功能
#
# 作者： sakurakugu
# 创建日期： 2025-10-07 23:33:00(UTC+8) 周一
# ============================================================

# 创建节假日管理静态库
add_library(mytodo_holiday STATIC
    # 节假日数据模型
    holiday_item.cpp
    holiday_item.h
    
    # 节假日管理器（数据获取、存储和查询）
    holiday_manager.cpp
    holiday_manager.h
)

# 查找所需的 Qt 组件
find_package(Qt6 REQUIRED COMPONENTS 
    Core        # 核心功能（QObject, QString, QDateTime等）
    Network     # 网络功能（API请求需要）
)

# 设置包含目录
target_include_directories(mytodo_holiday PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                    # 当前目录
    ${CMAKE_SOURCE_DIR}/cpp/foundation             # 基础模块
)

# 链接依赖库
target_link_libraries(mytodo_holiday
    PUBLIC
        Qt6::Core
        Qt6::Network
        mytodo_foundation    # 基础模块（网络请求、配置等）
)