#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QVariant>

/**
 * @class Settings
 * @brief 应用程序设置管理类
 *
 * Settings类提供了统一的接口来管理应用程序的设置，包括用户界面设置、
 * 网络设置和其他应用程序配置。所有设置都会持久化到本地存储。
 * 支持选择使用配置文件或注册表进行存储。
 */
class Settings : public QObject {
    Q_OBJECT

  public:
    /**
     * @enum StorageType
     * @brief 定义设置存储类型
     */
    enum StorageType {
        IniFile, ///< 使用INI配置文件存储
        Registry ///< 使用系统注册表存储
    };
    Q_ENUM(StorageType)

    explicit Settings(QObject *parent = nullptr, StorageType storageType = Registry);
    ~Settings();

    Q_INVOKABLE bool save(const QString &key, const QVariant &value); ///< 保存设置到配置文件
    Q_INVOKABLE QVariant get(const QString &key,
                             const QVariant &defaultValue = QVariant()); ///< 从配置文件读取设置
    Q_INVOKABLE void remove(const QString &key);                         ///< 移除设置
    Q_INVOKABLE bool contains(const QString &key);                       ///< 检查设置是否存在
    Q_INVOKABLE QStringList allKeys();                                   ///< 获取所有设置的键名
    Q_INVOKABLE void clearSettings();                                    ///< 清除所有设置
    Q_INVOKABLE void initializeDefaultServerConfig();                    ///< 初始化默认服务器配置
    Q_INVOKABLE StorageType getStorageType() const;                      ///< 获取当前存储类型

  private:
    QSettings *m_settings;     ///< 配置文件对象
    StorageType m_storageType; ///< 存储类型
};

#endif // SETTINGS_H