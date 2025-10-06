# HeaderCheck.cmake - 文件头规范校验模块
# 
# 该模块提供了校验 C/C++ 文件头规范的功能，包括检查文件头是否符合预定格式，
# 并可以自动修复一些简单的文件头规范问题。
# 
# 提供以下目标：
# - check_headers: 校验 C/C++ 文件头规范
# - fix_headers: 自动修复简单的文件头规范问题
#
# 使用方法：
# 1. 先包含本模块：include(tools/检查文件头/check_headers.cmake)
# 2. 构建校验目标：cmake --build build --target check_headers
# 3. 构建修复目标：cmake --build build --target fix_headers
#
# 作者： sakurakugu
# 创建日期： 2025-10-05 22:19:33(UTC+8) 周日
# 修改日期： 2025-10-05 23:32:12(UTC+8) 周日

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
    # COMMENT "正在校验 C/C++ 文件头规范..."
    COMMENT "Checking C/C++ headers..."
    VERBATIM
)

# 添加文件头规范自动修复目标
add_custom_target(fix_headers
    COMMAND ${Python3_EXECUTABLE} ${HEADER_CHECK_SCRIPT} --root ${CMAKE_SOURCE_DIR} --fix
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    # COMMENT "正在自动修复简单的文件头规范问题..."
    COMMENT "Fixing C/C++ headers..."
    VERBATIM
)

# 输出使用说明
message(STATUS "可用 “cmake --build build --target fix_headers / check_headers”  “校验并修复”或“校验”文件头规范")