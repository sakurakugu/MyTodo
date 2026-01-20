# ============================================================
# MyTodo 项目构建配置
# 现代化的 CMake 配置，支持 Qt6.10.0 和 C++23
#
# 作者： sakurakugu
# 首次创建日期： 2025-08-16 20:05:55(UTC+8) 周六
# 最后更新日期： 2025-10-06 02:13:23(UTC+8) 周一
# ============================================================

cmake_minimum_required(VERSION 3.16)

set(APP_NAME "MyTodo")
include("${CMAKE_SOURCE_DIR}/cmake/version/version.cmake") # 包含版本号处理模块

# 项目基本信息
project(${APP_NAME} 
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    DESCRIPTION "待办事项管理应用"
    HOMEPAGE_URL "https://github.com/sakurakugu/MyTodo"
    LANGUAGES C CXX
)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 23)                 # 使用 C++23 标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)        # 强制要求 C++23
set(CMAKE_CXX_EXTENSIONS OFF)              # 禁用编译器扩展，确保可移植性

# Qt 自动化工具
set(CMAKE_AUTORCC ON)                      # 自动处理资源文件
set(CMAKE_AUTOMOC ON)                      # 自动处理 MOC
set(CMAKE_AUTOUIC ON)                      # 自动处理 UIC

include(tools/检查文件头/check_headers.cmake)            # 包含文件头检查模块

# ============ 第三方依赖：SQLite3（设置可选内置构建） ============
# 提供选项：默认静态库，开启 JSON1/FTS5/RTREE
set(SQLITE3_VERSION "3.50.4" CACHE STRING "SQLite 语义化版本号")
set(SQLITE3_RELEASE_YEAR "2025" CACHE STRING "sqlite.org 上该版本的年份目录" FORCE)

# ============ 第三方依赖：nlohmann/json ============
set(NLOHMANN_JSON_VERSION "3.11.3" CACHE STRING "nlohmann/json 版本号")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake") # 包含自定义 CMake 模块路径
include(cmake/SQLite3.cmake)
include(cmake/nlohmann_json.cmake)
include(cmake/tomlplusplus.cmake)
include(cmake/stduuid.cmake)
add_sqlite3()
add_nlohmann_json()
add_tomlplusplus()
add_stduuid()


# 构建类型优化
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "选择构建类型" FORCE)
endif()

# 编译器特定优化
if(MSVC)
    # MSVC 编译器优化
    add_compile_options(/W4)               # 高警告级别
    add_compile_options(/permissive-)      # 严格标准符合性
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2)           # 优化
    endif()
else()
    # GCC/Clang 编译器优化
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    endif()
endif()

add_subdirectory(cpp/foundation)
add_subdirectory(cpp/foundation/logger)
add_subdirectory(cpp/foundation/datetime)
add_subdirectory(cpp/foundation/database)
add_subdirectory(cpp/domain)
add_subdirectory(cpp/modules/category)
add_subdirectory(cpp/modules/todo)
add_subdirectory(cpp/modules/setting)
add_subdirectory(cpp/modules/user)
add_subdirectory(cpp/modules/update_checker)
add_subdirectory(cpp/modules/holiday)
add_subdirectory(cpp/modules/backup)
add_subdirectory(cpp/app)
add_subdirectory(qml)
add_subdirectory(i18n)

# Qt6 组件配置
set(QT_COMPONENTS 
    Core                # 核心功能
    Widgets             # 桌面小组件支持
    Quick               # QML 引擎
    Network             # 网络功能
    Concurrent          # 并发处理
    Sql                 # SQL 数据库支持
    Gui                 # GUI 支持
)

# 链接库配置
set(PRIVATE_LIBRARIES
    mytodo_foundation
    mytodo_domain
    my_database
    mytodo_category
    mytodo_todo
    mytodo_setting
    mytodo_user
    mytodo_app
    Qt6::Widgets
    Qt6::Quick
    Qt6::Concurrent
    nlohmann_json::nlohmann_json
    stduuid::stduuid
)

# 单例组件
set_source_files_properties(
    qml/components/ThemeManager.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE # 主题管理器
)

# 资源文件配置
set(RESOURCE_FILES
    # 图标、图片等资源文件
    "./image/app_icon/icon.ico"          # 应用图标
    "./image/font_icon/iconfont.ttf"     # 字体图标
)

# 查找 Qt6 依赖
find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})

# Qt6 项目标准设置
qt_standard_project_setup(REQUIRES 6.10)

# 创建可执行文件目标
qt_add_executable(${APP_NAME} 
    main.cpp                        # 主入口文件
    "./image/app_icon/icon.rc"      # Windows 图标资源
)

# 设置目标属性
set_target_properties(${APP_NAME} PROPERTIES
    OUTPUT_NAME ${APP_NAME}            # 输出文件名
    DEBUG_POSTFIX "_debug"          # Debug 版本后缀
)

# 新增 QML 模块（会自动生成 ./.qmlls.ini）
qt_add_qml_module(${APP_NAME}
    URI ${APP_NAME}                              # QML 模块 URI
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}    # 模块版本
    QML_FILES ${QML_FILES}                       # QML 文件列表
    # 应用层 C++ 源文件已放入 mytodo_app 库，这里不再重复列出
    RESOURCES ${RESOURCE_FILES}                   # 资源文件
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml      # 输出目录
)

# 平台特定配置
# Windows 调试配置：在 VSCode + Ninja + Debug 模式下显示控制台
if(WIN32 AND "${CMAKE_GENERATOR}" MATCHES "Ninja" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(WIN32_EXEC_FLAG FALSE)          # 显示控制台窗口，便于调试
else()
    set(WIN32_EXEC_FLAG TRUE)           # 隐藏控制台窗口
endif()

# 安装路径配置
if (WIN32)
set(CMAKE_INSTALL_PREFIX "C:/Software/Apps/${APP_NAME}" CACHE PATH "安装路径前缀" FORCE)
endif()


# 目标平台属性配置
set_target_properties(${APP_NAME} PROPERTIES
    # macOS Bundle 配置
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.sakurakugu.mytodo"  # Bundle 标识符
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/platforms/macos/Info.plist.in
    
    # Windows 可执行文件配置
    WIN32_EXECUTABLE ${WIN32_EXEC_FLAG}
    
    # 通用属性
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# 链接库配置
target_link_libraries(${APP_NAME} PRIVATE ${PRIVATE_LIBRARIES})

# 包含目录配置
target_include_directories(${APP_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp                   # 根 cpp（放置全局头）
)

# TODO: 到时候打包安装部分改一下
# 安装配置
include(GNUInstallDirs)

# 安装目标文件
install(TARGETS MyTodo
    BUNDLE DESTINATION .                                    # macOS Bundle 安装位置
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}             # 可执行文件安装位置
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}             # 库文件安装位置
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}             # 静态库安装位置
)

# # 若 sqlite3 作为共享库构建，确保它也被安装到 bin/lib 中
# if(SQLITE3_BUILD_SHARED)
#     install(TARGETS sqlite3
#         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # 可执行文件安装位置
#         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # 库文件安装位置
#         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # 静态库安装位置
#     )
# endif()

# 安装 Qt6 运行时依赖（Windows）
if(WIN32)
    # 查找 Qt6 部署工具
    find_program(QT_DEPLOY_TOOL windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(QT_DEPLOY_TOOL)
        # 添加部署命令
        install(CODE "
            execute_process(
                COMMAND ${QT_DEPLOY_TOOL} --qmldir ${CMAKE_SOURCE_DIR}/qml $<TARGET_FILE:MyTodo>
                WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
            )
        ")
    endif()
endif()

# ---------------- 构建信息输出 ----------------
message(STATUS "====== ${APP_NAME} 构建配置 =====")
message(STATUS "项目版本: ${PROJECT_VERSION}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ 标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt6 版本: ${Qt6_VERSION}")
message(STATUS "CMake 版本: ${CMAKE_VERSION}")
message(STATUS "安装前缀: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "===========================")
