#include "config.h"
#include "default_value.h"

#if defined(Q_OS_WIN)
#include <QProcess>
#include <windows.h>
#endif

Config::Config(QObject *parent) : QObject(parent), m_config(std::make_unique<QSettings>("MyTodo", "TodoApp", this)) {
    qDebug() << "配置存放在:" << m_config->fileName();
}

Config::~Config() {
    // 确保所有设置都已保存
    if (m_config) {
        m_config->sync();
    }
}

/**
 * @brief 从配置文件读取设置
 * @param key 设置键名
 * @param defaultValue 默认值（如果设置不存在）
 * @return 设置值或错误信息
 */
std::expected<QVariant, ConfigError> Config::get(QStringView key, const QVariant &defaultValue) const noexcept {
    if (!m_config) {
        return std::unexpected(ConfigError::InvalidConfig);
    }

    try {
        QVariant value = m_config->value(key.toString(), defaultValue);

        if (isBooleanKey(key)) {
            value = processBooleanValue(value);
        }

        return value;
    } catch (...) {
        return std::unexpected(ConfigError::InvalidValue);
    }
}

/**
 * @brief 移除设置
 * @param key 设置键名
 * @return 操作结果或错误信息
 */
std::expected<void, ConfigError> Config::remove(QStringView key) noexcept {
    if (!m_config) {
        return std::unexpected(ConfigError::InvalidConfig);
    }

    try {
        m_config->remove(key.toString());
        return {};
    } catch (...) {
        return std::unexpected(ConfigError::InvalidValue);
    }
}

/**
 * @brief 检查设置是否存在
 * @param key 设置键名
 * @return 设置是否存在
 */
bool Config::contains(QStringView key) const noexcept {
    if (!m_config) {
        return false;
    }
    return m_config->contains(key.toString());
}

/**
 * @brief 获取所有设置的键名
 * @return 所有设置的键名列表
 */
QStringList Config::allKeys() const noexcept {
    if (!m_config) {
        return QStringList();
    }
    return m_config->allKeys();
}

/**
 * @brief 清除所有设置
 * @return 操作结果或错误信息
 */
std::expected<void, ConfigError> Config::clear() noexcept {
    if (!m_config) {
        return std::unexpected(ConfigError::InvalidConfig);
    }

    try {
        m_config->clear();
        return {};
    } catch (...) {
        return std::unexpected(ConfigError::SaveFailed);
    }
}

/**
 * @brief 获取配置文件路径
 * @return 配置文件路径
 */
QString Config::getConfigFilePath() const noexcept {
    if (!m_config) {
        return QString();
    }
    return m_config->fileName();
}

/**
 * @brief 打开配置文件路径
 * @return 操作结果或错误信息
 */
std::expected<void, ConfigError> Config::openConfigFilePath() const noexcept {
    if (!m_config) {
        return std::unexpected(ConfigError::InvalidConfig);
    }

    try {
        QString filePath = m_config->fileName();
        if (filePath.isEmpty()) {
            return std::unexpected(ConfigError::InvalidValue);
        }

#if defined(Q_OS_WIN)
        // Windows平台暂不支持
        return std::unexpected(ConfigError::InvalidValue);
#else
        if (QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) {
            return {};
        }
        return std::unexpected(ConfigError::SaveFailed);
#endif
    } catch (...) {
        return std::unexpected(ConfigError::InvalidValue);
    }
}

/**
 * @brief 检查键是否为布尔类型
 * @param key 设置键名
 * @return 是否为布尔类型键
 */
bool Config::isBooleanKey(QStringView key) const noexcept {
    const auto keyStr = key.toString().toStdString();
    return std::ranges::any_of(booleanKeys, [&keyStr](const auto &boolKey) { return keyStr == boolKey; });
}

/**
 * @brief 处理布尔值转换
 * @param value 原始值
 * @return 处理后的布尔值
 */
QVariant Config::processBooleanValue(const QVariant &value) const noexcept {
    if (value.typeId() == QMetaType::QString) {
        const QString strValue = value.toString().toLower();
        if (strValue == "true" || strValue == "1") {
            return QVariant(true);
        } else if (strValue == "false" || strValue == "0") {
            return QVariant(false);
        }
    }
    return value;
}