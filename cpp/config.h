#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QObject>
#include <QSettings>
#include <QVariant>
#include <toml.hpp>

/**
 * @class Config
 * @brief 应用程序设置管理类
 *
 * Settings类提供了统一的接口来管理应用程序的设置，包括用户界面设置、
 * 网络设置和其他应用程序配置。所有设置都会持久化到本地存储。
 * 支持选择使用配置文件或注册表进行存储。
 */
class Config : public QObject {
    Q_OBJECT

  public:
    /**
     * @enum StorageType
     * @brief 定义设置存储类型
     */
    enum StorageType {
        IniFile,  ///< 使用INI配置文件存储
        Registry, ///< 使用系统注册表存储
        TomlFile  ///< 使用TOML配置文件存储
    };
    Q_ENUM(StorageType)

    explicit Config(QObject *parent = nullptr, StorageType storageType = TomlFile);
    ~Config();

    Q_INVOKABLE bool save(const QString &key, const QVariant &value); ///< 保存设置到配置文件
    Q_INVOKABLE QVariant get(const QString &key,
                             const QVariant &defaultValue = QVariant()) const; ///< 从配置文件读取设置
    Q_INVOKABLE void remove(const QString &key);                         ///< 移除设置
    Q_INVOKABLE bool contains(const QString &key);                       ///< 检查设置是否存在
    Q_INVOKABLE QStringList allKeys();                                   ///< 获取所有设置的键名
    Q_INVOKABLE void clearSettings();                                    ///< 清除所有设置
    Q_INVOKABLE void initializeDefaultServerConfig();                    ///< 初始化默认服务器配置
    Q_INVOKABLE StorageType getStorageType() const;                      ///< 获取当前存储类型
    
    // 日志配置相关方法
    Q_INVOKABLE void setLogLevel(int level);                             ///< 设置日志级别
    Q_INVOKABLE int getLogLevel() const;                                 ///< 获取日志级别
    Q_INVOKABLE void setLogToFile(bool enabled);                        ///< 设置是否记录到文件
    Q_INVOKABLE bool getLogToFile() const;                              ///< 获取是否记录到文件
    Q_INVOKABLE void setLogToConsole(bool enabled);                     ///< 设置是否输出到控制台
    Q_INVOKABLE bool getLogToConsole() const;                           ///< 获取是否输出到控制台
    Q_INVOKABLE void setMaxLogFileSize(qint64 maxSize);                 ///< 设置最大日志文件大小
    Q_INVOKABLE qint64 getMaxLogFileSize() const;                       ///< 获取最大日志文件大小
    Q_INVOKABLE void setMaxLogFiles(int maxFiles);                      ///< 设置最大日志文件数量
    Q_INVOKABLE int getMaxLogFiles() const;                             ///< 获取最大日志文件数量
    Q_INVOKABLE QString getLogFilePath() const;                         ///< 获取日志文件路径
    Q_INVOKABLE void clearLogs();                                       ///< 清除所有日志文件

  private:
    StorageType m_storageType; ///< 存储类型
    QSettings *m_config;       ///< 配置文件对象
    QString m_tomlFilePath;    ///< TOML配置文件路径
    toml::value m_tomlData;    ///< TOML数据存储

    // TOML相关私有方法
    bool loadTomlFile();                                                                      ///< 加载TOML文件
    bool saveTomlFile();                                                                      ///< 保存TOML文件
    QVariant tomlValueToQVariant(const toml::value &value) const;                             ///< TOML值转QVariant
    toml::value qVariantToTomlValue(const QVariant &value);                                   ///< QVariant转TOML值
    void collectTomlKeys(const toml::value &value, const QString &prefix, QStringList &keys); ///< 递归收集TOML键
};

#endif // SETTINGS_H