/**
 * @file config.h
 * @brief Config类的头文件
 *
 * 该文件定义了Config类，用于管理应用程序的本地配置文件。
 *
 * @author Sakurakugu
 * @date 2025
 */
#pragma once

#include "default_value.h"

#include <QObject>
#include <QSettings>
#include <expected>

template <typename T>
concept ConfigValue = requires(T t) {
    QVariant{t}; // 可以转换为QVariant
};

// 错误类型枚举
enum class ConfigError {
    InvalidConfig, // 无效的配置文件
    KeyNotFound,   // 键不存在
    SaveFailed,    // 保存失败
    InvalidValue,  // 无效的值
    TypeMismatch,  // 类型不匹配
    GetFailed      // 获取失败
};

class Config : public QObject {
    Q_OBJECT

  public:
    // 单例模式 - 使用C++23的constexpr改进
    [[nodiscard]] static constexpr Config &GetInstance() noexcept {
        static Config instance;
        return instance;
    }

    // 禁用拷贝构造和赋值操作
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;
    Config(Config &&) = delete;
    Config &operator=(Config &&) = delete;

    // 向后兼容的保存方法（优先匹配）
    bool save(const QString &key, const QVariant &value) noexcept;

    // 使用std::expected替代bool返回值，提供更好的错误处理
    template <ConfigValue T>
    [[nodiscard]] std::expected<void, ConfigError> saveTyped(QStringView key, const T &value) noexcept;

    // 模板化的get方法，支持类型安全的获取
    template <ConfigValue T>
    [[nodiscard]] std::expected<T, ConfigError> getTyped(QStringView key, const T &defaultValue = T{}) const noexcept;

    // 传统的QVariant版本，保持向后兼容
    [[nodiscard]] std::expected<QVariant, ConfigError> get(QStringView key,
                                                           const QVariant &defaultValue = QVariant{}) const noexcept;
    QVariant get(const QString &key, const QVariant &defaultValue = QVariant{}) const noexcept;

    [[nodiscard]] std::expected<void, ConfigError> remove(QStringView key) noexcept;
    void remove(const QString &key) noexcept; // 向后兼容
    [[nodiscard]] bool contains(QStringView key) const noexcept;
    bool contains(const QString &key) const noexcept; // 向后兼容
    [[nodiscard]] QStringList allKeys() const noexcept;
    [[nodiscard]] std::expected<void, ConfigError> clearSettings() noexcept;
    void clear() noexcept; // 向后兼容的清除方法

    // 存储类型和路径管理相关方法
    [[nodiscard]] std::expected<void, ConfigError> openConfigFilePath() const noexcept;
    [[nodiscard]] QString getConfigFilePath() const noexcept;

    // 批量操作支持
    template <typename Container>
    [[nodiscard]] std::expected<void, ConfigError> saveBatch(const Container &keyValuePairs) noexcept;

  private:
    explicit Config(QObject *parent = nullptr);
    ~Config() override;

    std::unique_ptr<QSettings> m_config;
    static constexpr auto booleanKeys = DefaultValues::booleanKeys;

    // 辅助方法
    [[nodiscard]] bool isBooleanKey(QStringView key) const noexcept;
    [[nodiscard]] QVariant processBooleanValue(const QVariant &value) const noexcept;
};

// 模板实现
template <ConfigValue T>
inline std::expected<void, ConfigError> Config::saveTyped(QStringView key, const T &value) noexcept {
    if (!m_config) {
        return std::unexpected(ConfigError::InvalidConfig);
    }

    try {
        m_config->setValue(key.toString(), QVariant::fromValue(value));
        if (m_config->status() != QSettings::NoError) {
            return std::unexpected(ConfigError::SaveFailed);
        }
        return {};
    } catch (...) {
        return std::unexpected(ConfigError::SaveFailed);
    }
}

template <ConfigValue T>
inline std::expected<T, ConfigError> Config::getTyped(QStringView key, const T &defaultValue) const noexcept {
    if (!m_config) {
        return std::unexpected(ConfigError::InvalidConfig);
    }

    try {
        QVariant value = m_config->value(key.toString(), QVariant::fromValue(defaultValue));

        if constexpr (std::same_as<T, bool>) {
            if (isBooleanKey(key)) {
                return processBooleanValue(value);
            }
        }

        if (value.canConvert<T>()) {
            return value.value<T>();
        } else {
            return std::unexpected(ConfigError::TypeMismatch);
        }
    } catch (...) {
        return std::unexpected(ConfigError::GetFailed);
    }
}

template <typename Container>
inline std::expected<void, ConfigError> Config::saveBatch(const Container &keyValuePairs) noexcept {
    if (!m_config) {
        return std::unexpected(ConfigError::InvalidConfig);
    }

    try {
        for (const auto &[key, value] : keyValuePairs) {
            auto result = save(key, value);
            if (!result) {
                return result;
            }
        }
        return {};
    } catch (...) {
        return std::unexpected(ConfigError::SaveFailed);
    }
}
