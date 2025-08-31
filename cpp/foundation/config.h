/**
 * @file config.h
 * @brief Config类的头文件
 *
 * 该文件定义了Config类，用于管理应用程序的本地配置文件。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @version 2025-08-22 16:24:28(UTC+8) 周五
 */
#pragma once

#include <QDebug>
#include <QDir>
#include <QObject>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <toml.hpp>

class Config : public QObject {
    Q_OBJECT

  public:
    // 单例模式
    static Config &GetInstance() noexcept {
        static Config instance;
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;
    Config(Config &&) = delete;
    Config &operator=(Config &&) = delete;

    bool save(QStringView key, const QVariant &value) noexcept;
    QVariant get(QStringView key, const QVariant &defaultValue = QVariant{}) const noexcept;
    void remove(QStringView key) noexcept;
    bool contains(QStringView key) const noexcept;
    QStringList allKeys() const noexcept;
    void clear() noexcept;

    // 存储类型和路径管理相关方法
    bool openConfigFilePath() const noexcept;
    QString getConfigFilePath() const noexcept;

  private:
    explicit Config(QObject *parent = nullptr);
    ~Config() noexcept override;

    mutable toml::value m_tomlData; // 配置数据
    QString m_configFilePath;       // 配置文件路径
    mutable bool m_isChanged = false; // 配置是否已修改

    // 辅助方法
    void loadFromFile() const noexcept;
    bool saveToFile() const noexcept;
    QString getDefaultConfigPath() const noexcept;
    toml::value variantToToml(const QVariant &value) const noexcept;
    QVariant tomlToVariant(const toml::value &value) const noexcept;
};
