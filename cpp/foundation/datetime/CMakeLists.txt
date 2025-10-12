# ============================================================
# 日期时间：日期 / 时间 / 时间格式化
# 负责日期、时间和时间格式化的实现
#
# 作者： sakurakugu
# 创建日期： 2025-10-05 02:42:00(UTC+8) 周日
# 修改日期： 2025-10-06 00:53:59(UTC+8) 周一
# ============================================================
add_library(my_datetime STATIC
    date.cpp
    date.h
    datetime.cpp
    datetime.h
    time.cpp
    time.hpp
    formatter.cpp
    formatter.h
)

# 查找 Qt 库
find_package(Qt6 COMPONENTS Core QUIET)

# 将包含目录添加到库中
target_include_directories(my_datetime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}    # 允许其它模块用 #include "database.h" 等   
)

# 定义编译时宏，方便别的模块判断是否启用该库
target_compile_definitions(my_datetime PUBLIC
    MY_DATETIME_ENABLED
)