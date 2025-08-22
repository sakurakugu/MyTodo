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
    static Config &GetInstance() noexcept {
        static Config instance;
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;
    Config(Config &&) = delete;
    Config &operator=(Config &&) = delete;

    std::expected<void, ConfigError> save(QStringView key, const QVariant &value) noexcept;
    std::expected<QVariant, ConfigError> get(QStringView key,
                                                           const QVariant &defaultValue = QVariant{}) const noexcept;
    std::expected<void, ConfigError> remove(QStringView key) noexcept;
    bool contains(QStringView key) const noexcept;
    QStringList allKeys() const noexcept;
    std::expected<void, ConfigError> clear() noexcept;

    // 存储类型和路径管理相关方法
    std::expected<void, ConfigError> openConfigFilePath() const noexcept;
    QString getConfigFilePath() const noexcept;

  private:
    explicit Config(QObject *parent = nullptr);
    ~Config() noexcept override;

    std::unique_ptr<QSettings> m_config;
    static constexpr auto booleanKeys = DefaultValues::booleanKeys;

    // 辅助方法
    bool isBooleanKey(QStringView key) const noexcept;
    QVariant processBooleanValue(const QVariant &value) const noexcept;
};

// 模板实现
std::expected<void, ConfigError> Config::save(QStringView key, const QVariant &value) noexcept {
    if (!m_config) {
        return std::unexpected(ConfigError::InvalidConfig);
    }

    try {
        m_config->setValue(key.toString(), value);
        if (m_config->status() != QSettings::NoError) {
            return std::unexpected(ConfigError::SaveFailed);
        }
        return {};
    } catch (...) {
        return std::unexpected(ConfigError::SaveFailed);
    }
}
