/**
 * @file config.h
 * @brief Config类的头文件
 *
 * 该文件定义了Config类，用于管理应用程序的本地配置文件。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-08-31 15:07:38(UTC+8) 周日
 * @version 0.4.0
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

    bool save(QStringView key, const QVariant &value) noexcept;                              // 保存配置项
    QVariant get(QStringView key, const QVariant &defaultValue = QVariant{}) const noexcept; // 获取配置项
    void remove(QStringView key) noexcept;                                                   // 删除配置项
    bool contains(QStringView key) const noexcept;                                           // 检查配置项是否存在
    QStringList allKeys() const noexcept;                                                    // 获取所有配置项键
    void clear() noexcept;                                                                   // 清空所有配置项

    // 存储类型和路径管理相关方法
    bool openConfigFilePath() const noexcept;   // 打开配置文件路径
    QString getConfigFilePath() const noexcept; // 获取配置文件路径

  private:
    explicit Config(QObject *parent = nullptr);
    ~Config() noexcept override;

    mutable toml::value m_tomlData;   // 配置数据
    QString m_configFilePath;         // 配置文件路径
    mutable bool m_isChanged = false; // 配置是否已修改

    // 辅助方法
    void loadFromFile() const noexcept;                              // 从文件加载配置
    bool saveToFile() const noexcept;                                // 保存配置到文件
    QString getDefaultConfigPath() const noexcept;                   // 获取默认配置文件路径
    toml::value variantToToml(const QVariant &value) const noexcept; // Variant转换为Toml值
    QVariant tomlToVariant(const toml::value &value) const noexcept; // Toml值转换为Variant
};
