# MyTodo 项目构建配置
# 现代化的 CMake 配置，支持 Qt6 和 C++23

cmake_minimum_required(VERSION 3.16)

# 项目基本信息
project(MyTodo 
    VERSION 0.1.0
    DESCRIPTION "现代化的待办事项管理应用"
    HOMEPAGE_URL "https://github.com/sakurakugu/MyTodo"
    LANGUAGES CXX
)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 23)                 # 使用 C++23 标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)        # 强制要求 C++23
set(CMAKE_CXX_EXTENSIONS OFF)              # 禁用编译器扩展，确保可移植性

# Qt 自动化工具
set(CMAKE_AUTORCC ON)                      # 自动处理资源文件
set(CMAKE_AUTOMOC ON)                      # 自动处理 MOC
set(CMAKE_AUTOUIC ON)                      # 自动处理 UIC

# 构建类型优化
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
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


# Qt6 组件配置
set(QT_COMPONENTS 
    Core                # 核心功能
    Widgets             # 桌面小组件支持
    Quick               # QML 引擎
    Network             # 网络功能
    Concurrent          # 并发处理
)

# 链接库配置
set(PRIVATE_LIBRARIES
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::Network
    Qt6::Concurrent
    toml11::toml11
)

# 项目源文件配置
# 按功能模块组织源文件
set(CORE_SOURCES
    # 核心数据模型
    cpp/todoItem.cpp
    cpp/todoItem.h
    cpp/todoModel.cpp
    cpp/todoModel.h
)

set(NETWORK_SOURCES
    # 网络通信模块
    cpp/networkmanager.cpp
    cpp/networkmanager.h
)

set(UI_SOURCES
    # 用户界面模块
    cpp/mainWindow.cpp
    cpp/mainWindow.h
)

set(UTILITY_SOURCES
    # 工具和配置模块
    cpp/config.cpp
    cpp/config.h
    cpp/logger.cpp
    cpp/logger.h
)

# 合并所有源文件
set(PROJECT_SOURCES
    ${CORE_SOURCES}
    ${NETWORK_SOURCES}
    ${UI_SOURCES}
    ${UTILITY_SOURCES}
)

# QML 文件配置
# 按功能模块组织 QML 文件
set(QML_MAIN_FILES
    # 主要界面文件
    qml/Main.qml                    # 主窗口
    qml/MainPage.qml                # 主页面
    qml/Setting.qml                 # 设置页面
    qml/WidgetMode.qml              # 小组件模式
)

set(QML_COMPONENT_FILES
    # 可重用组件文件
    qml/components/CustomButton.qml       # 自定义按钮
    qml/components/CustomPopup.qml        # 自定义弹窗
    qml/components/TaskForm.qml           # 任务表单
    qml/components/ThemeManager.qml       # 主题管理器
    qml/components/ConfirmDialog.qml      # 通用确认对话框
    qml/components/LoginDialog.qml        # 登录对话框
    qml/components/MessageDialog.qml      # 消息提示对话框
    qml/components/LoginStatusDialogs.qml # 登录状态相关对话框
    qml/components/TodoDetailsDialog.qml  # 待办详情对话框
)

# 合并所有 QML 文件
set(QML_FILES
    ${QML_MAIN_FILES}
    ${QML_COMPONENT_FILES}
)

# 国际化配置（预留）
set(TRANSLATION_FILES
    # 未来支持的翻译文件
    # i18n/MyTodo_en.ts      # 英文
    # i18n/MyTodo_zh_CN.ts   # 简体中文
    # i18n/MyTodo_ja.ts      # 日文
)

# 资源文件配置
set(RESOURCE_FILES
    # 图标、图片等资源文件
    # resources/icons/app.ico
    # resources/images/logo.png
)

# 添加外部库 toml11
include(FetchContent)
FetchContent_Declare(
    toml11
    GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
    GIT_TAG        v4.4.0
)
FetchContent_MakeAvailable(toml11)

# 查找 Qt6 依赖
find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})

# Qt6 项目标准设置
qt_standard_project_setup(REQUIRES 6.8)

# 创建可执行文件目标
qt_add_executable(appMyTodo
    main.cpp                        # 主入口文件
    ${PROJECT_SOURCES}              # 所有 C++ 源文件
    # "./resources/icon.rc"         # Windows 图标资源（可选）
)

# 设置目标属性
set_target_properties(appMyTodo PROPERTIES
    OUTPUT_NAME "MyTodo"            # 输出文件名
    DEBUG_POSTFIX "_debug"              # Debug 版本后缀
)

# 添加 QML 模块
qt_add_qml_module(appMyTodo
    URI MyTodo                      # QML 模块 URI
    VERSION 1.0                     # 模块版本
    QML_FILES ${QML_FILES}          # QML 文件列表
    SOURCES ${PROJECT_SOURCES}      # C++ 源文件
    RESOURCES ${RESOURCE_FILES}     # 资源文件
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/MyTodo  # 输出目录
)

# 国际化翻译配置（预留）
# 当需要支持多语言时，取消注释以下配置
# qt_add_translations(appMyTodo
#     RESOURCE_PREFIX /qt/qml/MyTodo/i18n  # 资源前缀
#     TS_FILES ${TRANSLATION_FILES}        # 翻译文件
#     QM_FILES_OUTPUT_VARIABLE qm_files    # 输出变量
# )

# 平台特定配置
# Windows 调试配置：在 VSCode + Ninja + Debug 模式下显示控制台
if(WIN32 AND "${CMAKE_GENERATOR}" MATCHES "Ninja" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(WIN32_EXEC_FLAG FALSE)          # 显示控制台窗口，便于调试
else()
    set(WIN32_EXEC_FLAG TRUE)           # 隐藏控制台窗口
endif()

# 安装路径配置
if (WIN32)
set(CMAKE_INSTALL_PREFIX "C:/Software/Apps/MyTodo" CACHE PATH "Install path prefix" FORCE)
endif()


# 目标平台属性配置
set_target_properties(appMyTodo PROPERTIES
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
target_link_libraries(appMyTodo PRIVATE ${PRIVATE_LIBRARIES})

# 包含目录配置
target_include_directories(appMyTodo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp     # C++ 头文件目录
)

# 编译定义
target_compile_definitions(appMyTodo PRIVATE
    QT_QML_DEBUG                        # QML 调试支持
    $<$<CONFIG:Debug>:DEBUG_BUILD>      # Debug 构建标志
    $<$<CONFIG:Release>:RELEASE_BUILD>  # Release 构建标志
)

# 安装配置
include(GNUInstallDirs)

# 安装目标文件
install(TARGETS appMyTodo
    BUNDLE DESTINATION .                                    # macOS Bundle 安装位置
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}             # 可执行文件安装位置
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}             # 库文件安装位置
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}             # 静态库安装位置
)

# 安装 Qt6 运行时依赖（Windows）
if(WIN32)
    # 查找 Qt6 部署工具
    find_program(QT_DEPLOY_TOOL windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(QT_DEPLOY_TOOL)
        # 添加部署命令
        install(CODE "
            execute_process(
                COMMAND ${QT_DEPLOY_TOOL} --qmldir ${CMAKE_SOURCE_DIR}/qml $<TARGET_FILE:appMyTodo>
                WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
            )
        ")
    endif()
endif()

# 构建信息输出
message(STATUS "====== MyTodo 构建配置 =====")
message(STATUS "项目版本: ${PROJECT_VERSION}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ 标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt6 版本: ${Qt6_VERSION}")
message(STATUS "安装前缀: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "===========================")
