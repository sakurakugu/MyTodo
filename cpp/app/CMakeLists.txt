# ============================================================
# MyTodo 应用层包装/管理器模块构建配置
# 提供对 QML 暴露的各类 Manager（QmlTodoManager / QmlCategoryManager / QmlUserAuthManager / QmlGlobalData / QmlSetting）
# 这些类组合并协调各底层 domain / modules / foundation 的能力，聚合为 UI 友好的接口。
#
# 作者： sakurakugu
# 创建日期： 2025-10-06 09:54:00(UTC+8) 周一
# ============================================================

# 以静态库形式构建，便于后续拆分或被测试目标链接
add_library(mytodo_app STATIC
    qml_category.cpp
    qml_category.h
    qml_global_data.cpp
    qml_global_data.h
    qml_setting.cpp
    qml_setting.h
    qml_todo.cpp
    qml_todo.h
    qml_user_auth.cpp
    qml_user_auth.h
    qml_update_checker.cpp
    qml_update_checker.h
    qml_holiday.cpp
    qml_holiday.h
)

# 需要的 Qt 组件：Core / Gui（模型基类等）/ Network（透传网络操作）/ Sql（数据持久化）
find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Sql)

# 公开包含目录，供其它 target 简单 include "qml_todo.h" 等
target_include_directories(mytodo_app PUBLIC
    ${CMAKE_SOURCE_DIR}/cpp               # 允许以 modules/xxx 形式包含
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/cpp/foundation    # 访问 config.h logger.h 等
    ${CMAKE_SOURCE_DIR}/cpp/modules/todo  # 访问模型 / 数据存储 / 同步相关类
    ${CMAKE_SOURCE_DIR}/cpp/modules/category
    ${CMAKE_SOURCE_DIR}/cpp/modules/setting
    ${CMAKE_SOURCE_DIR}/cpp/modules/user
    ${CMAKE_SOURCE_DIR}/cpp/modules/update_checker
    ${CMAKE_SOURCE_DIR}/cpp/modules/holiday
    ${CMAKE_SOURCE_DIR}/cpp/domain        # 访问基类（若 manager 未来需要）
)

# 链接依赖库（两类：Qt + 项目内部模块）
target_link_libraries(mytodo_app
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        mytodo_foundation
        mytodo_domain
        mytodo_category
        mytodo_todo
        mytodo_setting
        mytodo_user
        mytodo_update_checker
        mytodo_holiday
)
