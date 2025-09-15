# MyTodo 项目构建配置
# 现代化的 CMake 配置，支持 Qt6 和 C++23
#
# 作者： sakurakugu
# 首次创建日期： 2025-08-16 20:05:55(UTC+8) 周六
# 最后更新日期： 2025-09-06 16:55:02(UTC+8) 周六

cmake_minimum_required(VERSION 3.16)

# 包含版本号处理模块
include("${CMAKE_SOURCE_DIR}/cpp/script/version.cmake")

# 项目基本信息
project(MyTodo 
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    DESCRIPTION "待办事项管理应用"
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

# 添加外部库 toml++
include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

# Qt6 组件配置
set(QT_COMPONENTS 
    Core                # 核心功能
    Widgets             # 桌面小组件支持
    Quick               # QML 引擎
    Network             # 网络功能
    Concurrent          # 并发处理
    Sql                 # SQL 数据库支持
    LinguistTools       # 国际化工具
)

# 链接库配置
set(PRIVATE_LIBRARIES
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::Network
    Qt6::Concurrent
    Qt6::Sql
    tomlplusplus::tomlplusplus
)

# QML 文件配置
# 按功能模块组织 QML 文件
set(QML_MAIN_FILES
    # 主要界面文件
    qml/Main.qml                    # 入口文件
    qml/HomePage.qml                # 主窗口
    qml/SettingPage.qml               # 登录窗口
    qml/WidgetMode.qml              # 小窗口模式
)

set(QML_COMPONENT_FILES
    # 可重用组件文件
    qml/components/BaseDialog.qml                   # 基础对话框
    qml/components/ModalDialog.qml                  # 模态对话框
    qml/components/InputDialog.qml                  # 输入对话框
    qml/components/LoginStatusDialogs.qml           # 登录状态相关对话框

    qml/components/MainBackground.qml               # 主背景
    qml/components/TitleBar.qml                     # 标题栏
    
    qml/components/TodoListContainer.qml            # 待办列表显示区域
    qml/components/ThemeManager.qml                 # 主题管理器
    qml/components/IconButton.qml                   # 图标按钮
    qml/components/WindowDragHandler.qml            # 窗口拖拽处理
    qml/components/ClickBlocker.qml                 # 点击拦截器
    qml/components/DateTimePicker.qml               # 日期时间选择器
    qml/components/TodoCategoryManager.qml          # 待办事项分类管理器
    qml/components/WindowResizeHandler.qml          # 待办事项项管理器
    qml/components/Divider.qml                      # 分割线
    qml/components/RecurrenceSelector.qml           # 重复选择器
    qml/components/CustomSwitch.qml                 # 自定义开关
    qml/components/CustomButton.qml                 # 通用按钮
    qml/components/CustomTextInput.qml              # 自定义文本输入框
    qml/components/CustomComboBox.qml              # 自定义下拉列表框
    qml/components/CustomSpinBox.qml               # 自定义数字输入框
    qml/components/CustomTextEdit.qml              # 自定义文本编辑框
    qml/components/CustomCheckBox.qml              # 自定义复选框
    qml/components/CustomPopup.qml                 # 自定义弹出窗口
    qml/components/ControlRow.qml                   # 自定义控件行

    qml/TestPage.qml # 测试页面
)

# 单例组件
set_source_files_properties(
    qml/components/ThemeManager.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE # 主题管理器
)

# 项目源文件配置
set(ITEM_SOURCES
    # 数据库条目模块
    cpp/todos/items/todo_item.cpp
    cpp/todos/items/todo_item.h
    cpp/todos/items/categorie_item.cpp
    cpp/todos/items/categorie_item.h
)

set(CORE_SOURCES
    # 核心数据模型
    cpp/todos/todo/todo_manager.cpp
    cpp/todos/todo/todo_manager.h
    cpp/todos/todo/todo_sync_server.cpp
    cpp/todos/todo/todo_sync_server.h
    cpp/todos/todo/todo_data_storage.cpp
    cpp/todos/todo/todo_data_storage.h
    cpp/todos/todo/todo_sorter.cpp
    cpp/todos/todo/todo_sorter.h
    cpp/todos/todo/todo_filter.cpp
    cpp/todos/todo/todo_filter.h

    cpp/todos/category/category_manager.h
    cpp/todos/category/category_manager.cpp
    cpp/todos/category/category_sync_server.cpp
    cpp/todos/category/category_sync_server.h
    cpp/todos/category/category_data_storage.cpp
    cpp/todos/category/category_data_storage.h

    cpp/todos/base_sync_server.cpp
    cpp/todos/base_sync_server.h
)

set(NETWORK_SOURCES
    # 网络通信模块
    cpp/foundation/network_request.cpp
    cpp/foundation/network_request.h
    cpp/foundation/network_proxy.cpp
    cpp/foundation/network_proxy.h
    cpp/user_auth.cpp
    cpp/user_auth.h
)

set(UI_SOURCES
    # 用户界面模块
    cpp/global_state.cpp
    cpp/global_state.h
)

set(UTILITY_SOURCES
    # 工具和配置模块
    cpp/foundation/config.cpp
    cpp/foundation/config.h
    cpp/foundation/logger.cpp
    cpp/foundation/logger.h
    cpp/setting.cpp
    cpp/setting.h
    cpp/foundation/database.cpp
    cpp/foundation/database.h
    cpp/default_value.h
)

# 合并所有源文件
set(PROJECT_SOURCES
    ${ITEM_SOURCES}
    ${CORE_SOURCES}
    ${NETWORK_SOURCES}
    ${UI_SOURCES}
    ${UTILITY_SOURCES}
)

# 合并所有 QML 文件
set(QML_FILES
    ${QML_MAIN_FILES}
    ${QML_COMPONENT_FILES}
)

# 国际化翻译的语言列表
set(QT_I18N_TRANSLATED_LANGUAGES 
    en
    zh_CN
    ja
)
# 国际化翻译文件的路径
set(I18N_FILES
    i18n/MyTodo_en.ts
    i18n/MyTodo_zh_CN.ts
    i18n/MyTodo_ja.ts
)
# 然后在主目录输入：`cmake --build build --target update_translations` 以更新翻译文件

# 资源文件配置
set(RESOURCE_FILES
    # 图标、图片等资源文件
    "./image/app_icon/icon.ico"          # 应用图标
    "./image/font_icon/iconfont.ttf"     # 字体图标
)

# 查找 Qt6 依赖
find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})

# Qt6 项目标准设置
qt_standard_project_setup(REQUIRES 6.9)

# 创建可执行文件目标
qt_add_executable(MyTodo
    main.cpp                        # 主入口文件
    ${PROJECT_SOURCES}              # 所有 C++ 源文件
    "./image/app_icon/icon.rc"      # Windows 图标资源
)

# 设置目标属性
set_target_properties(MyTodo PROPERTIES
    OUTPUT_NAME "MyTodo"            # 输出文件名
    DEBUG_POSTFIX "_debug"              # Debug 版本后缀
)

# 新增 QML 模块
# TODO: 到时候将qml/components和qml/分开
qt_add_qml_module(MyTodo
    URI MyTodo                      # QML 模块 URI
    VERSION 1.0                     # 模块版本
    QML_FILES ${QML_FILES}          # QML 文件列表
    SOURCES ${PROJECT_SOURCES}      # C++ 源文件
    RESOURCES ${RESOURCE_FILES}     # 资源文件
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/MyTodo  # 输出目录  # TODO: 到时候这个地址要改，至少MyTodo要没有
)

# 国际化翻译配置
qt_add_translations(MyTodo
    # 最终路径：qrc:/i18n/MyTodo_*.qm
    RESOURCE_PREFIX /i18n                # 资源前缀
    TS_FILES ${I18N_FILES}               # 翻译文件
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
set(CMAKE_INSTALL_PREFIX "C:/Software/Apps/MyTodo" CACHE PATH "安装路径前缀" FORCE)
endif()


# 目标平台属性配置
set_target_properties(MyTodo PROPERTIES
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
target_link_libraries(MyTodo PRIVATE ${PRIVATE_LIBRARIES})

# 包含目录配置
target_include_directories(MyTodo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp     # C++ 头文件目录
    ${INCLUDE_DIR}                      # 由 script 自动设置的生成文件目录
)

# 安装配置
include(GNUInstallDirs)

# 安装目标文件
install(TARGETS MyTodo
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
                COMMAND ${QT_DEPLOY_TOOL} --qmldir ${CMAKE_SOURCE_DIR}/qml $<TARGET_FILE:MyTodo>
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
