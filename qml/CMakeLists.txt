## =============================================
# QML 子模块 CMake 配置
# 负责收集全部 QML/资源文件并暴露变量给父级
# 输出变量:
#   MYTODO_QML_FILES        -> 所有 QML 文件集合
#   MYTODO_QML_RESOURCES    -> 资源文件（若后续需要可扩展）
#   MYTODO_QML_SINGLETONS   -> 需要标记为单例的 QML 文件列表
#
# 使用方式（父 CMakeLists.txt 中示例）：
#   add_subdirectory(qml)
#   qt_add_qml_module(MyTodo ... QML_FILES ${MYTODO_QML_FILES} ...)
#
# 后续扩展: 如需新增模块，可在此追加列表或拆分成更细子目录。
## =============================================

# 主要界面文件
set(QML_MAIN_FILES
    qml/Main.qml
    qml/HomePage.qml
    qml/SettingPage.qml
    qml/WidgetMode.qml
    # qml/TestPage.qml  # 测试页面
)

# 可复用组件
set(QML_COMPONENT_FILES
    qml/components/BaseDialog.qml
    qml/components/ModalDialog.qml
    qml/components/InputDialog.qml
    qml/components/LoginStatusDialogs.qml
    qml/components/MainBackground.qml
    qml/components/TitleBar.qml
    qml/components/ThemeManager.qml
    qml/components/IconButton.qml
    qml/components/WindowDragHandler.qml
    qml/components/ClickBlocker.qml
    qml/components/DateTimePicker.qml
    qml/components/WindowResizeHandler.qml
    qml/components/Divider.qml
    qml/components/RecurrenceSelector.qml
    qml/components/CustomSwitch.qml
    qml/components/CustomButton.qml
    qml/components/CustomTextInput.qml
    qml/components/CustomComboBox.qml
    qml/components/CustomSpinBox.qml
    qml/components/CustomTextEdit.qml
    qml/components/CustomCheckBox.qml
    qml/components/CustomPopup.qml
    qml/components/ControlRow.qml
)

# 待办模块相关
set(QML_TODOS_FILES
    qml/todos/TodoListContainer.qml
    qml/todos/TodoCategoryManager.qml
)

# 合并全部 QML 文件
set(QML_FILES
    ${QML_MAIN_FILES}
    ${QML_COMPONENT_FILES}
    ${QML_TODOS_FILES}
    PARENT_SCOPE              # 让其它在父作用域可见
)

# set_source_files_properties(
#     ${CMAKE_CURRENT_SOURCE_DIR}/components/ThemeManager.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE # 主题管理器
#     ${CMAKE_SOURCE_DIR}/qml/components/ThemeManager.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE # 主题管理器
# )

