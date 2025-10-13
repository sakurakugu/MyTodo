# ============================================================
# 数据库：数据库 / SQL 查询
# 负责数据库访问和 SQL 查询的实现
#
# 作者： sakurakugu
# 创建日期： 2025-10-05 02:42:00(UTC+8) 周日
# 修改日期： 2025-10-06 00:53:59(UTC+8) 周一
# ============================================================
add_library(my_database STATIC
    database.cpp
    database.h
    sql_query.cpp
    sql_query.h
    sql_value.h
)

# 查找 Qt 库
find_package(Qt6 COMPONENTS Core QUIET)

# 将包含目录添加到库中
target_include_directories(my_database PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}    # 允许其它模块用 #include "database.h" 等   
    ${INCLUDE_DIR}                 # 允许包含生成的 version.h 
)

# 如果编译失败，检查主CMake中导入的顺序是否正确
# 比如确保先导入 my_datetime 再导入 my_database

# 链接 SQLite3 库 和 nlohmann_json::nlohmann_json
target_link_libraries(my_database PUBLIC 
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
)
# 如果找到了 Qt6，则链接 Qt6::Core
if(Qt6_FOUND)
    target_link_libraries(my_database PUBLIC Qt6::Core)
endif()
# 如果找到了 my_datetime，则链接 my_datetime
if(TARGET my_datetime)
    target_link_libraries(my_database PUBLIC my_datetime)
endif()
# 如果找到了 my_logger，则链接 my_logger
if(TARGET my_logger)
    target_link_libraries(my_database PUBLIC my_logger)
endif()
# 如果找到了 stduuid::stduuid，则链接 stduuid::stduuid
if(TARGET stduuid::stduuid)
    target_link_libraries(my_database PUBLIC stduuid::stduuid)
endif()

# 定义编译时宏，方便别的模块判断是否启用该库
target_compile_definitions(my_database PUBLIC
    MY_DATABASE_ENABLED
)