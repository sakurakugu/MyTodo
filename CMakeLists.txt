cmake_minimum_required(VERSION 3.16)

project(MyTodo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)                 # 设置 C++ 标准为 C++23      
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)                      # 自动处理资源文件


# 设置私有库变量 MYPRIVATE, 储存需要链接的库
set(MYPRIVATE
    Qt6::Quick
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::Network
)

# 设置 待包含的 Qt 组件
set(QT_COMPONENTS 
    Core
    Widgets
    Quick
    Network
)

# 设置项目的源文件
set(PROJECT_SOURCES
    cpp/todoItem.cpp
    cpp/todoItem.h
    cpp/todoModel.cpp
    cpp/todoModel.h
    cpp/settings.cpp
    cpp/settings.h
    cpp/mainWindow.cpp
    cpp/mainWindow.h

)

# 设置 QML 复用组件的文件路径
set(MYQML_COMPONENTS
    # qml/components/Cell.qml
    # qml/components/MyButton.qml
)

# 设置 QML 文件的路径
set(MYQML_FILES
    qml/Main.qml
    qml/MainPage.qml
    qml/Setting.qml
    qml/WidgetMode.qml
    ${MYQML_COMPONENTS}
)

# 设置国际化翻译文件的路径
set(MYI18N_FILES
    # i18n/qml_en.ts
    # i18n/qml_zh_CN.ts
    # i18n/qml_ja.ts
)

# 设置资源文件的路径
set(RESOURCES
)

find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appMyTodo
    main.cpp
    ${PROJECT_SOURCES}
    # "./image/icon.rc" # 添加图标资源
)

qt_add_qml_module(appMyTodo
    URI MyTodo
    VERSION 1.0
    QML_FILES
        ${MYQML_FILES}
        SOURCES ${PROJECT_SOURCES}
        RESOURCES ${RESOURCES}
)

# 国际化翻译
# qt_add_translations(appLinkGame
#     # 最终路径：qrc:/qt/qml/Translated/i18n/qml_*.qm
#     RESOURCE_PREFIX /qt/qml/Translated/i18n
#     TS_FILES ${MYI18N_FILES}
# )

# 如果在 Windows 下使用 VSCode 与 Debug 模式，设置 WIN32_EXEC_FLAG 为 FALSE
# 为了希望在 VSCode 中调试时能够在终端中看到输出，所以需要将 WIN32_EXEC_FLAG 设为 FALSE
if(WIN32 AND "${CMAKE_GENERATOR}" MATCHES "Ninja" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(WIN32_EXEC_FLAG FALSE) # VSCode 默认 Ninja
else()
    set(WIN32_EXEC_FLAG TRUE)
endif()

# Qt 在 iOS 上会自动设置 MACOSX_BUNDLE_GUI_IDENTIFIER
# 如果你正在为 iOS 或 macOS 开发，你应该考虑手动设置一个明确的、固定的 bundle 标识符。
set_target_properties(appMyTodo PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appMyTodo
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE ${WIN32_EXEC_FLAG}  # 改为 FALSE 以可以在vscode终端显示qDebug（TRUE：不弹出控制台窗口，FALSE 相反）
)

target_link_libraries(appMyTodo
    PRIVATE ${MYPRIVATE}
)

include(GNUInstallDirs)
install(TARGETS appMyTodo
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
