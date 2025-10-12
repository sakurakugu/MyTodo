# ============================================================
# 数据库：数据库 / SQL 查询
# 负责数据库访问和 SQL 查询的实现
#
# 作者： sakurakugu
# 创建日期： 2025-10-05 02:42:00(UTC+8) 周日
# 修改日期： 2025-10-06 00:53:59(UTC+8) 周一
# ============================================================
add_library(mytodo_database STATIC
    database.cpp
    database.h
    sql_query.cpp
    sql_query.h
    sql_value.h
)

# 查找 Qt 库
find_package(Qt6 COMPONENTS Core QUIET)

# 将包含目录添加到库中
target_include_directories(mytodo_database PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}    # 允许其它模块用 #include "database.h" 等   
    ${INCLUDE_DIR}                 # 允许包含生成的 version.h 
)

# 链接 Qt 库和 SQLite3 库
target_link_libraries(mytodo_database
    PUBLIC
        Qt6::Core
        SQLite::SQLite3
        my_logger
)

# 定义编译时宏，方便别的模块判断是否启用该库
target_compile_definitions(mytodo_database PUBLIC
    MY_DATABASE_ENABLED
)