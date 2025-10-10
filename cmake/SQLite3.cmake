# 使用 CMake 获取并构建 SQLite3 库
# 示例:
#   set(SQLITE3_VERSION "3.50.4" CACHE STRING "SQLite3 语义化版本号")                    # 版本号
#   set(SQLITE3_RELEASE_YEAR "2025" CACHE STRING "sqlite.org 上该版本的年份目录" FORCE)
#   option(SQLITE3_BUILD_SHARED "是否构建为共享库" OFF)                                  # 是否构建共享库
#   option(SQLITE3_ENABLE_JSON1 "启用 JSON1 扩展" ON)                                   # 是否启用 JSON1 扩展
#   option(SQLITE3_ENABLE_FTS5  "启用 FTS5 扩展" ON)                                    # 是否启用 FTS5 扩展
#   option(SQLITE3_ENABLE_RTREE "启用 RTREE 扩展" ON)                                   # 是否启用 RTREE 扩展
#   include(cmake/SQLite3.cmake)                                                       # 包含本文件
#   add_sqlite3()                                                                      # 将 SQLite3 库添加到项目中
#
# 可以通过设置 SQLITE3_AMALGAMATION_URL 来覆盖默认的下载 URL。
# 例如:
#   set(SQLITE3_AMALGAMATION_URL "https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip" CACHE STRING "sqlite 归并版下载 URL")
# 
# 注意事项：
# 1. `cmake/FetchSQLite3.cmake` 是通过 FetchContent 从 sqlite.org 下载 amalgamation 压缩包，然后创建 `sqlite3` 目标，并根据开关启用 JSON1/FTS5/RTREE。
# 2. 通过顶层 `CMakeLists.txt` 暴露上述开关；若启用，将链接 `SQLite::SQLite3`。
# 3. 安装时会将生成的库与头文件安装到标准目录（bin/lib/include）。
# 4. 记得在 project() 中添加C语言支持，否则可能会因为没有启用C语言而导致sqlite3.c被当作C++编译，从而引发链接错误。例如：
#    project(MyProject LANGUAGES C CXX)

# 若遇到下载 404，请确认 `SQLITE3_VERSION` 与 `SQLITE3_RELEASE_YEAR` 是否匹配；必要时直接指定 `SQLITE3_AMALGAMATION_URL`。

include_guard(GLOBAL)

include(FetchContent)

# 可选：使用系统自带的 SQLite3（若可用）。默认关闭，保持当前行为（下载并内置构建）。
option(SQLITE3_USE_SYSTEM "若为 ON 则优先查找并使用系统 SQLite3，而非下载归并版构建" OFF)

# 可选：设置 SQLite3 C 标准（不强制全局）。仅在本目标上应用。
set(SQLITE3_C_STANDARD "11" CACHE STRING "为 sqlite3.c 目标设置的 C 标准（如 99/11/17）")

# 可选：用户自定义传递给 sqlite3 的额外编译宏（以分号分隔）。例如：SQLITE3_EXTRA_DEFINES="SQLITE_OMIT_LOAD_EXTENSION;SQLITE_USE_URI"。
set(SQLITE3_EXTRA_DEFINES "" CACHE STRING "传递给 sqlite3 的额外宏，分号分隔")

# 可选：构建 SQLite shell（sqlite3 命令行工具），默认关闭；开启需可执行入口和控制台。
option(SQLITE3_BUILD_SHELL "构建 sqlite3 命令行 shell（可执行文件）" OFF)

# 将语义化版本转换为归并版的整数版本码（若未提供）
# 例如: 3.50.4 -> 3500400
function(_sqlite3_semver_to_amalgamation out_var semver)
    string(REPLACE "." ";" _parts "${semver}")
    list(GET _parts 0 _MAJOR)
    list(GET _parts 1 _MINOR)
    # PATCH 可能包含额外数字，如 3.50.4.0，仅取前三段中的第三段
    list(GET _parts 2 _PATCH)
    if(_PATCH STREQUAL "")
        set(_PATCH 0)
    endif()
    # 归并版整数版本码 = M*1_000_000 + N*10_000 + P*100
    math(EXPR _AMALG "${_MAJOR} * 1000000 + ${_MINOR} * 10000 + ${_PATCH} * 100")
    set(${out_var} "${_AMALG}" PARENT_SCOPE)
endfunction()

if(NOT DEFINED SQLITE3_VERSION)
    set(SQLITE3_VERSION "3.50.4" CACHE STRING "SQLite3 语义化版本号")
endif()

if(NOT DEFINED SQLITE3_AMALGAMATION_VERSION)
    _sqlite3_semver_to_amalgamation(_calc "${SQLITE3_VERSION}")
    set(SQLITE3_AMALGAMATION_VERSION "${_calc}" CACHE STRING "SQLite3 归并版整数版本码" FORCE)
endif()

if(NOT DEFINED SQLITE3_BUILD_SHARED)
    option(SQLITE3_BUILD_SHARED "是否构建为共享库" OFF)
endif()

if(NOT DEFINED SQLITE3_ENABLE_JSON1)
    option(SQLITE3_ENABLE_JSON1 "启用 JSON1 扩展" ON)
endif()
if(NOT DEFINED SQLITE3_ENABLE_FTS5)
    option(SQLITE3_ENABLE_FTS5  "启用 FTS5 扩展" ON)
endif()
if(NOT DEFINED SQLITE3_ENABLE_RTREE)
    option(SQLITE3_ENABLE_RTREE "启用 RTREE 扩展" ON)
endif()

# SQLite 官方网站的发布资源按年份划分目录；允许通过变量覆盖默认 URL。
# 控制是否自动生成下载 URL，默认开启。
if(NOT DEFINED SQLITE3_URL_AUTO)
    option(SQLITE3_URL_AUTO "自动计算 sqlite 归并版下载 URL" ON)
endif()

if(SQLITE3_URL_AUTO)
    # 根据版本推导发布年份（若未显式设置）。
    if(NOT DEFINED SQLITE3_RELEASE_YEAR)
        # 经验规则：3.47.* 归属 2025+，3.46.* 归属 2024
        string(REGEX MATCH "^([0-9]+)\.([0-9]+)\.([0-9]+)" _m "${SQLITE3_VERSION}")
        set(_REL_YEAR 2024)
        if(CMAKE_MATCH_2)
            math(EXPR _MINOR "${CMAKE_MATCH_2}")
            if(_MINOR GREATER_EQUAL 47)
                set(_REL_YEAR 2025)
            endif()
        endif()
        set(SQLITE3_RELEASE_YEAR "${_REL_YEAR}" CACHE STRING "sqlite.org 上该版本对应的年份目录" FORCE)
    endif()

    # 始终强制更新 URL，以避免使用到缓存中的过期值
    set(SQLITE3_AMALGAMATION_URL "https://www.sqlite.org/${SQLITE3_RELEASE_YEAR}/sqlite-amalgamation-${SQLITE3_AMALGAMATION_VERSION}.zip" CACHE STRING "sqlite 归并版下载 URL" FORCE)
endif()

# 将 SQLite3 归并版源码下载并添加为库，供项目使用
function(add_sqlite3)
    # 若用户希望优先使用系统 SQLite3，则尝试 find_package
    if(SQLITE3_USE_SYSTEM)
        find_path(SQLITE3_SYS_INCLUDE_DIR sqlite3.h)
        find_library(SQLITE3_SYS_LIBRARY NAMES sqlite3)
        if(SQLITE3_SYS_INCLUDE_DIR AND SQLITE3_SYS_LIBRARY)
            add_library(sqlite3 UNKNOWN IMPORTED)
            set_target_properties(sqlite3 PROPERTIES
                IMPORTED_LOCATION "${SQLITE3_SYS_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_SYS_INCLUDE_DIR}"
            )
            add_library(SQLite::SQLite3 ALIAS sqlite3)
            message(STATUS "SQLite3: 使用系统库 -> ${SQLITE3_SYS_LIBRARY}")
            return()
        else()
            message(WARNING "SQLite3: 未找到系统库，回退到下载归并版构建")
        endif()
    endif()
    # 使用独立的子构建名称，避免先前尝试的缓存影响
    set(_proj sqlite_amalgamation)
    FetchContent_Declare(
        ${_proj}
        URL               ${SQLITE3_AMALGAMATION_URL}
        SOURCE_SUBDIR     ""      # 压缩包根目录包含 sqlite3.c/.h
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_GetProperties(${_proj})
    if(NOT ${_proj}_POPULATED)
        FetchContent_MakeAvailable(${_proj})
    endif()

    # 压缩包解压后的目录名称通常为 sqlite-amalgamation-<ver>
    # 在源目录下查找符合该模式的第一个子目录
    file(GLOB _amalg_dirs "${${_proj}_SOURCE_DIR}/sqlite-amalgamation-*")
    list(LENGTH _amalg_dirs _dir_count)
    if(_dir_count EQUAL 0)
        # 某些环境会直接解压到 SOURCE_DIR
        set(_SQLITE_SRC_DIR "${${_proj}_SOURCE_DIR}")
    else()
        list(GET _amalg_dirs 0 _SQLITE_SRC_DIR)
    endif()

    if(NOT EXISTS "${_SQLITE_SRC_DIR}/sqlite3.c")
        message(FATAL_ERROR "在归并版源码目录未找到 sqlite3.c: ${_SQLITE_SRC_DIR}")
    endif()

    # 根据选项决定构建静态库或共享库
    set(_lib_type STATIC)
    if(SQLITE3_BUILD_SHARED)
        set(_lib_type SHARED)
    endif()

    add_library(sqlite3 ${_lib_type}
        "${_SQLITE_SRC_DIR}/sqlite3.c"
    )

    # 确保以 C 语言编译 sqlite3.c（项目可能只启用了 CXX，会导致被当作 C++ 编译从而符号被修饰）
    set_source_files_properties("${_SQLITE_SRC_DIR}/sqlite3.c" PROPERTIES LANGUAGE C)
    # 对该目标关闭 Qt 的自动处理，避免对纯 C 源触发不必要的 AUTOGEN
    set_target_properties(sqlite3 PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
        LINKER_LANGUAGE C
        C_STANDARD ${SQLITE3_C_STANDARD}
        C_EXTENSIONS OFF
    )

    target_include_directories(sqlite3 PUBLIC "${_SQLITE_SRC_DIR}")

    # 通用编译宏定义
    target_compile_definitions(sqlite3
        PRIVATE
            SQLITE_THREADSAFE=1              # 启用线程安全
            SQLITE_DEFAULT_FOREIGN_KEYS=1    # 默认启用外键约束
    )

    if(SQLITE3_ENABLE_JSON1)
        target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_JSON1) # 启用 JSON1 扩展
    endif()
    if(SQLITE3_ENABLE_FTS5)
        target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_FTS5)  # 启用 FTS5 全文检索扩展
    endif()
    if(SQLITE3_ENABLE_RTREE)
        target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_RTREE) # 启用 R*Tree 扩展
    endif()

    if(MSVC)
        target_compile_definitions(sqlite3 PRIVATE _CRT_SECURE_NO_WARNINGS) # 关闭 MSVC 安全警告
        target_compile_options(sqlite3 PRIVATE /W3)                          # 适度的编译警告等级
    else()
        # sqlite 归并版源码警告很多，这里关闭编译警告
        target_compile_options(sqlite3 PRIVATE -w)
        # MinGW/Clang 常见设置：确保按 C 规则处理
        if(MINGW)
            target_compile_definitions(sqlite3 PRIVATE _WIN32_WINNT=0x0601)
        endif()
    endif()

    if(WIN32 AND SQLITE3_BUILD_SHARED)
        # 在 Windows 平台构建 DLL 时，确保正确导出接口
        target_compile_definitions(sqlite3 PRIVATE SQLITE_API=__declspec(dllexport))
    endif()

    # 创建别名目标，便于在项目中以 SQLite::SQLite3 引用
    add_library(SQLite::SQLite3 ALIAS sqlite3)

    # 用户自定义额外宏（可选）
    if(SQLITE3_EXTRA_DEFINES)
        separate_arguments(_EXTRA_DEFS NATIVE_COMMAND "${SQLITE3_EXTRA_DEFINES}")
        foreach(_d IN LISTS _EXTRA_DEFS)
            if(NOT _d STREQUAL "")
                target_compile_definitions(sqlite3 PRIVATE ${_d})
            endif()
        endforeach()
    endif()

    # 安装规则
    include(GNUInstallDirs)
    install(TARGETS sqlite3
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(FILES "${_SQLITE_SRC_DIR}/sqlite3.h"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    # 可选：构建 sqlite3 shell（需要 shell.c）
    if(SQLITE3_BUILD_SHELL)
        if(EXISTS "${_SQLITE_SRC_DIR}/shell.c")
            add_executable(sqlite3_shell "${_SQLITE_SRC_DIR}/shell.c")
            set_target_properties(sqlite3_shell PROPERTIES
                AUTOMOC OFF AUTOUIC OFF AUTORCC OFF
                LINKER_LANGUAGE C
                C_STANDARD ${SQLITE3_C_STANDARD}
                C_EXTENSIONS OFF
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            )
            target_include_directories(sqlite3_shell PRIVATE "${_SQLITE_SRC_DIR}")
            target_link_libraries(sqlite3_shell PRIVATE sqlite3)
            if(WIN32)
                target_compile_definitions(sqlite3_shell PRIVATE _CRT_SECURE_NO_WARNINGS)
            endif()
            install(TARGETS sqlite3_shell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
        else()
            message(WARNING "SQLite3: 未找到 shell.c，跳过 shell 构建")
        endif()
    endif()
endfunction()
