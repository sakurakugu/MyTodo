# ============================================================
# MyTodo 待办事项管理模块构建配置
# 负责待办事项的数据模型、存储、同步和查询功能
#
# 作者： sakurakugu
# 创建日期： 2025-10-06 00:27:00(UTC+8) 周一
# 修改日期： 2025-10-06 02:13:23(UTC+8) 周一
# ============================================================

# 创建待办事项管理静态库
add_library(mytodo_todo STATIC
    # 待办事项数据模型
    todo_item.cpp
    todo_item.h
    
    # 待办事项视图模型（QML界面显示）
    todo_model.cpp
    todo_model.h
    
    # 待办事项数据存储（本地存储和文件导入导出）
    todo_data_storage.cpp
    todo_data_storage.h
    
    # 待办事项同步服务（与服务器同步）
    todo_sync_server.cpp
    todo_sync_server.h
    
    # 待办事项查询器（过滤、排序、搜索功能）
    todo_queryer.cpp
    todo_queryer.h
)

# 查找所需的 Qt 组件
find_package(Qt6 REQUIRED COMPONENTS 
    Core        # 核心功能（QObject, QString, QDateTime等）
    Gui         # GUI功能（QAbstractListModel等）
    Network     # 网络功能（同步服务器需要）
)

# 设置包含目录
target_include_directories(mytodo_todo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                    # 当前目录
    ${CMAKE_SOURCE_DIR}/cpp/foundation             # 基础模块
    ${CMAKE_SOURCE_DIR}/cpp/domain                # 领域模块
)

# 链接依赖库
target_link_libraries(mytodo_todo
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        mytodo_foundation    # 基础模块（配置、日志、工具等）
        mytodo_domain        # 领域模块（BaseDataStorage等）
        mytodo_database      # 数据库模块（SQLite数据库连接等）
)
