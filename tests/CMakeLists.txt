# MyTodo 项目测试配置
# 测试框架：Qt Test
# 支持的测试类型：单元测试、集成测试

cmake_minimum_required(VERSION 3.16)

# 项目信息
project(MyTodoTests VERSION 1.0.0 LANGUAGES CXX)

# 启用测试功能
enable_testing()

# C++ 标准配置
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译器特定配置
if(MSVC)
    # MSVC 编译器配置
    add_compile_options(
        /Zc:__cplusplus          # 启用正确的 __cplusplus 宏
        /utf-8                   # 使用 UTF-8 编码
        /W4                      # 启用高级别警告
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od)  # 禁用优化
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang 编译器配置
    add_compile_options(
        -Wall                    # 启用常见警告
        -Wextra                  # 启用额外警告
        -Wpedantic               # 启用严格标准检查
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-O0 -g)  # 禁用优化，启用调试信息
    endif()
endif()

# Qt6 组件查找
find_package(Qt6 REQUIRED COMPONENTS 
    Test                     # Qt 测试框架
    Core                     # Qt 核心模块
    Quick                    # Qt Quick 模块
    Network                  # Qt 网络模块
    Concurrent               # Qt 并发模块
)

# 测试源文件配置
set(TEST_SOURCES
    test_todoitem.cpp        # TodoItem 类单元测试
    test_todomodel.cpp       # TodoModel 类单元测试
    test_networkmanager.cpp  # NetworkManager 类单元测试
)

# 主项目源文件依赖
# 注意：这些文件需要与测试一起编译
set(PROJECT_SOURCES
    ../cpp/todoitem.cpp      # Todo 项目数据模型
    ../cpp/todomodel.cpp     # Todo 列表模型
    ../cpp/networkmanager.cpp # 网络管理器
    ../cpp/settings.cpp      # 设置管理实现
)

# 包含目录配置
set(INCLUDE_DIRECTORIES
    ../cpp                   # 主项目 C++ 头文件目录
    ${CMAKE_CURRENT_SOURCE_DIR}  # 当前测试目录
)

# 为每个测试源文件创建独立的可执行文件
foreach(test_source ${TEST_SOURCES})
    # 提取测试名称（不包含扩展名）
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # 创建测试可执行文件
    qt_add_executable(${test_name}
        ${test_source}           # 当前测试源文件
        ${PROJECT_SOURCES}       # 主项目依赖源文件
    )
    
    # 设置包含目录
    target_include_directories(${test_name} PRIVATE ${INCLUDE_DIRECTORIES})
    
    # 链接 Qt6 库
    target_link_libraries(${test_name}
        PRIVATE
        Qt6::Test               # Qt 测试框架
        Qt6::Core               # Qt 核心功能
        Qt6::Quick              # Qt Quick 支持
        Qt6::Network            # 网络功能
        Qt6::Concurrent         # 并发支持
    )
    
    # 设置测试属性
    set_target_properties(${test_name} PROPERTIES
        OUTPUT_NAME "${test_name}"           # 输出文件名
        DEBUG_POSTFIX "_d"                  # Debug 版本后缀
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"  # 输出目录
    )
    
    # 添加编译定义
    target_compile_definitions(${test_name} PRIVATE
        QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}"  # 测试构建目录
        $<$<CONFIG:Debug>:QT_QML_DEBUG>     # Debug 模式下启用 QML 调试
    )
    
    # 注册测试到 CTest
    add_test(
        NAME ${test_name}                   # 测试名称
        COMMAND ${test_name}                # 测试命令
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}  # 工作目录
    )
    
    # 设置测试属性
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 30                          # 测试超时时间（秒）
        LABELS "unit"                       # 测试标签
    )
    
    # 输出测试信息
    message(STATUS "配置测试: ${test_name}")
endforeach()

# 添加自定义测试目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS ${TEST_SOURCES}
    COMMENT "运行所有单元测试"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# 测试覆盖率配置（仅在 Debug 模式下）
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    option(ENABLE_TEST_COVERAGE "启用测试覆盖率" OFF)
    if(ENABLE_TEST_COVERAGE)
        foreach(test_source ${TEST_SOURCES})
            get_filename_component(test_name ${test_source} NAME_WE)
            target_compile_options(${test_name} PRIVATE --coverage)
            target_link_libraries(${test_name} PRIVATE --coverage)
        endforeach()
        
        # 添加覆盖率报告目标
        add_custom_target(coverage
            COMMAND lcov --capture --directory . --output-file coverage.info
            COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND lcov --list coverage.info
            COMMAND genhtml coverage.info --output-directory coverage_html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "生成测试覆盖率报告"
        )
    endif()
endif()

# 测试配置总结
message(STATUS "=== 测试配置总结 ===")
message(STATUS "测试数量: ${CMAKE_CURRENT_LIST_DIR}")
list(LENGTH TEST_SOURCES test_count)
message(STATUS "测试数量: ${test_count}")
message(STATUS "测试输出目录: ${CMAKE_BINARY_DIR}/tests")
message(STATUS "===================")