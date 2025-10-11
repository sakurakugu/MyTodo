# ============================================================
# 基础底座：日志
# 负责基础功能的实现，包括日志记录
#
# 作者： sakurakugu
# 创建日期： 2025-10-11 20:47:00 (UTC+8) 周六
# 修改日期： 2025-10-11 23:01:43 (UTC+8) 周六
# ============================================================
add_library(my_logger STATIC
    logger.cpp
    logger.h
    log_stream.h
)

# 查找 Qt 库
find_package(Qt6 COMPONENTS Core QUIET )

# 将包含目录添加到库中
target_include_directories(my_logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}    # 允许其它模块用 #include "logger.h" 等    
    ${INCLUDE_DIR}                 # 允许包含生成的 version.h
)

if(Qt6_FOUND)
# 链接 Qt 库
target_link_libraries(my_logger
    PUBLIC
        Qt6::Core
)
endif()