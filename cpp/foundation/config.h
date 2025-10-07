/**
 * @file config.h
 * @brief Config类的头文件
 *
 * 该文件定义了Config类，用于管理应用程序的本地配置文件。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-10 16:24:14(UTC+8) 周三
 */
#pragma once
#include <QString>
#include <QVariant>
#include <toml++/toml.hpp>

class Config {
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

    void save(const QString &key, const QVariant &value);                 // 保存配置项
    QVariant get(const QString &key) const;                               // 获取配置项
    QVariant get(const QString &key, const QVariant &defaultValue) const; // 获取配置项（带默认值）
    void remove(const QString &key);                                      // 删除配置项
    const toml::node *find(const QString &key) const noexcept;            // 查找配置项
    bool contains(const QString &key) const noexcept;                     // 检查配置项是否存在
    void clear();                                                         // 清空所有配置项

    // 批量操作接口
    void setBatch(const QVariantMap &values);  // 批量设置配置项
    void saveBatch(const QVariantMap &values); // 批量保存配置项

    // JSON 相关功能
    std::string exportToJson(const std::vector<std::string> &excludeKeys = {}) const; // 导出配置到JSON字符串
    bool importFromJson(const std::string &jsonContent, bool replaceAll);             // 从JSON字符串导入到当前配置
    bool exportToJsonFile(const std::string &filePath,
                          const std::vector<std::string> &excludeKeys = {}); // 导出配置到JSON文件
    bool importFromJsonFile(const std::string &filePath, bool replaceAll);   // 从JSON文件导入到当前配置
    bool JsonToToml(const std::string &jsonContent, toml::table *table);     // JSON字符串 转换为 TOML

    // 配置文件位置切换
    enum class Location {
        ApplicationPath, // 应用程序所在路径
        AppDataLocal     // AppData/Local路径
    };

    bool setConfigLocation(Location location, bool overwrite = false);             // 设置配置文件位置
    Location getConfigLocation() const;                                            // 获取配置文件位置
    std::string getConfigLocationPath(Location location) const;                    // 获取配置文件位置路径
    bool migrateConfigToLocation(Location targetLocation, bool overwrite = false); // 迁移配置文件到指定位置

    // 存储类型和路径管理相关方法
    bool openConfigFilePath() const;   // 打开配置文件路径
    QString getConfigFilePath() const; // 获取配置文件路径

  private:
    explicit Config();
    ~Config() noexcept;

    // 辅助方法
    void loadFromFile();                                              // 从文件加载配置
    bool saveToFile() const;                                          // 保存配置到文件
    std::string getDefaultConfigPath() const;                         // 获取默认配置文件路径
    void findExistingConfigFile();                                    // 查找现有配置文件并更新位置
    std::unique_ptr<toml::node> variantToToml(const QVariant &value); // QVariant转换为Toml值
    QVariant tomlToVariant(const toml::node *node) const;             // Toml值转换为QVariant

    mutable std::mutex m_mutex; // 线程安全保护
    toml::table m_config;       // 配置数据
    std::string m_filePath;     // 配置文件路径
#if defined(QT_DEBUG)
    Location m_configLocation = Location::ApplicationPath;
#else
    Location m_configLocation = Location::AppDataLocal;
#endif
};
