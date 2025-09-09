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
#include <mutex>
#include <optional>
#include <string_view>
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

    void save(const QString &key, const QVariant &value, std::string_view comment = ""); // 保存配置项
    void set(const QString &key, const QVariant &value, std::string_view comment = "");  // 设置配置项
    std::optional<QVariant> get(const QString &key) const;                               // 获取配置项
    QVariant get(const QString &key, const QVariant &defaultValue) const;                // 获取配置项（带默认值）
    void remove(const QString &key);                                                     // 删除配置项
    bool contains(const QString &key) const noexcept;                                    // 检查配置项是否存在
    QStringList allKeys() const;                                                         // 获取所有配置项键
    void clear();                                                                        // 清空所有配置项

    // 批量操作接口
    void setBatch(const QVariantMap &values);  // 批量设置配置项
    void saveBatch(const QVariantMap &values); // 批量保存配置项

    // JSON 导出功能
    std::string exportToJson(const QStringList &excludeKeys = QStringList()) const;
    bool exportToJsonFile(const QString &filePath,
                          const QStringList &excludeKeys = QStringList()) const; // 导出到JSON文件

    // 配置文件位置切换
    enum class ConfigLocation {
        ApplicationPath, // 应用程序所在路径
        AppDataLocal     // AppData/Local路径
    };

    void setConfigLocation(ConfigLocation location);
    ConfigLocation getConfigLocation() const;
    std::string getConfigLocationPath(ConfigLocation location) const;

    // 存储类型和路径管理相关方法
    bool openConfigFilePath() const;   // 打开配置文件路径
    QString getConfigFilePath() const; // 获取配置文件路径

  private:
    explicit Config(QObject *parent = nullptr);
    ~Config() noexcept override;

    mutable std::mutex m_mutex;       // 线程安全保护
    toml::table m_config;             // 配置数据
    std::string m_filePath;               // 配置文件路径
    mutable bool m_needsSave = false; // 延迟写入标记
#if defined(QT_DEBUG)
    ConfigLocation m_configLocation = ConfigLocation::ApplicationPath;
#else
    ConfigLocation m_configLocation = ConfigLocation::AppDataLocal;
#endif

    // 辅助方法
    void loadFromFile();                                                                     // 从文件加载配置
    bool saveToFile() const;                                                                 // 保存配置到文件
    std::string getDefaultConfigPath() const;                                                    // 获取默认配置文件路径
    std::unique_ptr<toml::node> variantToToml(const QVariant &value);                        // QVariant转换为Toml值
    QVariant tomlToVariant(const toml::node *node) const;                                    // Toml值转换为QVariant
    void collectKeys(const toml::table &tbl, const QString &prefix, QStringList &out) const; // 递归收集所有键
};
