# HeaderCheck.cmake - 文件头规范校验模块
# 
# 提供以下目标：
# - check_headers: 校验 C/C++ 文件头规范
# - fix_headers: 自动修复简单的文件头规范问题

# 查找Python3解释器
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# 定义校验脚本路径
set(HEADER_CHECK_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/check_headers.py")

# 检查校验脚本是否存在
if(NOT EXISTS ${HEADER_CHECK_SCRIPT})
    message(WARNING "文件头校验脚本不存在: ${HEADER_CHECK_SCRIPT}")
    return()
endif()

# 添加文件头规范校验目标
add_custom_target(check_headers
    COMMAND ${Python3_EXECUTABLE} ${HEADER_CHECK_SCRIPT} --root ${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "正在校验 C/C++ 文件头规范..."
    VERBATIM
)

# 添加文件头规范自动修复目标
add_custom_target(fix_headers
    COMMAND ${Python3_EXECUTABLE} ${HEADER_CHECK_SCRIPT} --root ${CMAKE_SOURCE_DIR} --fix
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "正在自动修复简单的文件头规范问题..."
    VERBATIM
)

# 输出使用说明
message(STATUS "可用 “cmake --build build --target fix_headers / check_headers”  “校验并修复”或“校验”文件头规范")