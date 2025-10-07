/**
 * @file qml_setting.h
 * @brief QmlSetting 包装类，将 Setting 单例对 QML 暴露的接口隔离出来
 *
 * 设计目的：
 * - 降低 QML 对核心 Setting 实现的直接耦合
 * - 后续可在不影响 QML 的情况下重构 Setting 内部逻辑或权限控制
 *
 * 该文件定义了QmlSetting类，负责管理应用程序的设置。
 *
 * @author Sakurakugu
 * @date 2025-10-06 01:32:19(UTC+8) 周一
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */
#pragma once

#include <QObject>
#include <QVariant>

#include "setting.h" // 底层业务单例

class QmlSetting : public QObject {
    Q_OBJECT
  public:
    explicit QmlSetting(QObject *parent = nullptr);

    // ---- 直接数据存取 ----
    Q_INVOKABLE void save(const QString &key, const QVariant &value);
    Q_INVOKABLE QVariant get(const QString &key, const QVariant &defaultValue = QVariant()) const;
    Q_INVOKABLE void remove(const QString &key);
    Q_INVOKABLE bool contains(const QString &key) const;
    Q_INVOKABLE void clear();

    // ---- 文件 / 路径 ----
    Q_INVOKABLE bool openConfigFilePath() const;
    Q_INVOKABLE QString getConfigFilePath() const;

    // ---- JSON 导入导出 ----
    Q_INVOKABLE bool exportConfigToJsonFile(const QString &filePath);
    Q_INVOKABLE bool importConfigFromJsonFile(const QString &filePath, bool replaceAll);
    Q_INVOKABLE bool exportDatabaseToJsonFile(const QString &filePath);
    Q_INVOKABLE bool importDatabaseFromJsonFile(const QString &filePath, bool replaceAll);

    // ---- 配置文件位置管理与迁移 ----
    Q_INVOKABLE int getConfigLocation() const;                     // 获取当前配置文件位置枚举值
    Q_INVOKABLE QString getConfigLocationPath(int location) const; // 获取指定位置路径
    Q_INVOKABLE bool migrateConfigLocation(int targetLocation, bool overwriteExisting);

    // ---- 代理配置 ----
    Q_INVOKABLE void setProxyType(int type);
    Q_INVOKABLE int getProxyType() const;
    Q_INVOKABLE void setProxyHost(const QString &host);
    Q_INVOKABLE QString getProxyHost() const;
    Q_INVOKABLE void setProxyPort(int port);
    Q_INVOKABLE int getProxyPort() const;
    Q_INVOKABLE void setProxyUsername(const QString &username);
    Q_INVOKABLE QString getProxyUsername() const;
    Q_INVOKABLE void setProxyPassword(const QString &password);
    Q_INVOKABLE QString getProxyPassword() const;
    Q_INVOKABLE void setProxyEnabled(bool enabled);
    Q_INVOKABLE bool getProxyEnabled() const;
    Q_INVOKABLE void setProxyConfig(bool enableProxy, int type, const QString &host, int port, const QString &username,
                                    const QString &password);

    // ---- 服务器配置 ----
    Q_INVOKABLE bool isHttpsUrl(const QString &url) const;
    Q_INVOKABLE void updateServerConfig(const QString &baseUrl);

  signals:
    void baseUrlChanged();
};
