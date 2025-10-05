# ============================================================
# MyTodo 类别管理模块构建配置
# 负责待办事项分类的数据模型、存储和同步功能
#
# 作者： sakurakugu
# 创建日期： 2025-10-06 00:26:00
# ============================================================

# 创建类别管理静态库
add_library(mytodo_category STATIC
    # 类别数据模型
    category_item.cpp
    category_item.h
    
    # 类别视图模型（QML界面显示）
    category_model.cpp
    category_model.h
    
    # 类别数据存储（本地存储和文件导入导出）
    category_data_storage.cpp
    category_data_storage.h
    
    # 类别同步服务（与服务器同步）
    category_sync_server.cpp
    category_sync_server.h
)

# 查找所需的 Qt 组件
find_package(Qt6 REQUIRED COMPONENTS 
    Core        # 核心功能（QObject, QString, QDateTime等）
    Gui         # GUI功能（QAbstractListModel等）
    Network     # 网络功能（同步服务器需要）
    Sql         # SQL数据库支持
)

# 设置包含目录
target_include_directories(mytodo_category PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                    # 当前目录
    ${CMAKE_SOURCE_DIR}/cpp/foundation             # 基础模块
    ${CMAKE_SOURCE_DIR}/cpp/domain                # 领域模块
)

# 链接依赖库
target_link_libraries(mytodo_category
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Sql
        mytodo_foundation    # 基础模块（配置、日志、数据库等）
        mytodo_domain        # 领域模块（BaseDataStorage等）
)