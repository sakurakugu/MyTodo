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
#include <toml++/toml.hpp>
// import tomlplusplus; // Qt暂时不推荐用
class Config : public QObject {
    Q_OBJECT

  public:
    // 单例模式
    static Config &GetInstance() {
        static Config instance;
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;
    Config(Config &&) = delete;
    Config &operator=(Config &&) = delete;

    void save(const QString &key, const QVariant &value);                              // 保存配置项
    void set(const QString &key, const QVariant &value);                               // 设置配置项
    QVariant get(const QString &key, const QVariant &defaultValue = QVariant{}) const; // 获取配置项
    void remove(const QString &key);                                                   // 删除配置项
    bool contains(const QString &key) const;                                           // 检查配置项是否存在
    QStringList allKeys() const;                                                       // 获取所有配置项键
    void clear();                                                                      // 清空所有配置项

    // 存储类型和路径管理相关方法
    bool openConfigFilePath() const;   // 打开配置文件路径
    QString getConfigFilePath() const; // 获取配置文件路径

  private:
    explicit Config(QObject *parent = nullptr);
    ~Config() noexcept override;

    toml::table m_config; // 配置数据
    QString m_filePath;   // 配置文件路径

    // 辅助方法
    void loadFromFile();                                                                     // 从文件加载配置
    bool saveToFile() const;                                                                 // 保存配置到文件
    QString getDefaultConfigPath() const;                                                    // 获取默认配置文件路径
    std::unique_ptr<toml::node> variantToToml(const QVariant &value);                        // QVariant转换为Toml值
    QVariant tomlToVariant(const toml::node *node) const;                                    // Toml值转换为QVariant
    void collectKeys(const toml::table &tbl, const QString &prefix, QStringList &out) const; // 递归收集所有键
};
