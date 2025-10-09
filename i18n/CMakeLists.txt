# 定义语言显示名称映射
set(LANGUAGE_DISPLAY_NAMES
    "zh_CN:简体中文"
    "en:English"
    "ja:日本語"
)

# 生成语言列表内容
set(LANG_CODE "")

# 国际化翻译的语言列表
set(QT_I18N_TRANSLATED_LANGUAGES "")
set(LANGUAGE_LIST_CONTENT "")
foreach(lang_item IN LISTS LANGUAGE_DISPLAY_NAMES)
    string(REPLACE ":" ";" parts "${lang_item}")
    list(GET parts 0 lang_code)
    list(GET parts 1 lang_name)
    list(APPEND QT_I18N_TRANSLATED_LANGUAGES "${lang_code}")
    set(LANGUAGE_LIST_CONTENT "${LANGUAGE_LIST_CONTENT}\n    {\"${lang_code}\", \"${lang_name}\"},")
endforeach()

# 生成语言配置头文件
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/i18n_languages.h.in"
    "${CMAKE_BINARY_DIR}/generated/i18n_languages.h"
    @ONLY
)

# 将生成的头文件目录添加到包含路径
include_directories("${CMAKE_BINARY_DIR}/generated")



# 查找 Qt6 LinguistTools 组件
find_package(Qt6 REQUIRED COMPONENTS
    Core                # 核心功能
    LinguistTools       # 国际化工具
)

# 国际化翻译配置
qt_add_translations(${APP_NAME}
    # 最终路径：qrc:/i18n/${APP_NAME}_*.qm
    RESOURCE_PREFIX /                # 资源前缀
    TS_FILE_BASE ${APP_NAME}         # 翻译文件前缀 ${APP_NAME}_${LANG}.ts
    LUPDATE_OPTIONS  
        -source-language zh_CN       # 源语言为简体中文
        -no-obsolete                 # 不包含已过时的翻译
    # QM_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/  # 输出目录
)

# ---------------- 翻译文件配置 ----------------
message(STATUS "在主目录输入 “cmake --build build --target update_translations” 以更新翻译文件")
