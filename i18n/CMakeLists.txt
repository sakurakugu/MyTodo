# ============================================================
# MyTodo 国际化模块构建配置
# 负责管理翻译文件和国际化资源
#
# 作者： sakurakugu
# 创建日期： 2025-10-07 18:12:00(UTC+8) 周二
# ============================================================

# 国际化翻译的语言列表
set(QT_I18N_TRANSLATED_LANGUAGES
    en          # 英语
    zh_CN       # 简体中文
    ja          # 日语
)

# 查找 Qt6 LinguistTools 组件
find_package(Qt6 REQUIRED COMPONENTS
    Core                # 核心功能
    LinguistTools       # 国际化工具
)

# 国际化翻译配置
qt_add_translations(${APP_NAME}
    # 最终路径：qrc:/i18n/MyTodo_*.qm
    RESOURCE_PREFIX /                # 资源前缀
    TS_FILE_BASE ${APP_NAME}         # 翻译文件前缀 ${APP_NAME}_${LANG}.ts
    LUPDATE_OPTIONS  
        -source-language zh_CN       # 源语言为简体中文
        -no-obsolete                 # 不包含已过时的翻译
    # QM_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/  # 输出目录
)
