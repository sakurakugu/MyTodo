/**
 * @file setting_manager.h
 * @brief SettingManager 包装类，将 Setting 单例对 QML 暴露的接口隔离出来
 *
 * 设计目的：
 * - 降低 QML 对核心 Setting 实现的直接耦合
 * - 后续可在不影响 QML 的情况下重构 Setting 内部逻辑或权限控制
 * 
 * 该文件定义了SettingManager类，负责管理应用程序的设置。
 * 
 * @author Sakurakugu
 * @date 2025-10-06 01:32:19(UTC+8) 周一
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */
#pragma once

#include <QObject>
#include <QVariant>

#include "setting.h" // 底层业务单例

class SettingManager : public QObject {
    Q_OBJECT
  public:
    explicit SettingManager(QObject *parent = nullptr) : QObject(parent) {
        // 透传底层信号
        QObject::connect(&Setting::GetInstance(), &Setting::baseUrlChanged, this, &SettingManager::baseUrlChanged);
    }

    // ---- 直接数据存取 ----
    Q_INVOKABLE void save(const QString &key, const QVariant &value) { Setting::GetInstance().save(key, value); }
    Q_INVOKABLE QVariant get(const QString &key, const QVariant &defaultValue = QVariant()) const {
        return Setting::GetInstance().get(key, defaultValue);
    }
    Q_INVOKABLE void remove(const QString &key) { Setting::GetInstance().remove(key); }
    Q_INVOKABLE bool contains(const QString &key) const { return Setting::GetInstance().contains(key); }
    Q_INVOKABLE void clear() { Setting::GetInstance().clear(); }

    // ---- 文件 / 路径 ----
    Q_INVOKABLE bool openConfigFilePath() const { return Setting::GetInstance().openConfigFilePath(); }
    Q_INVOKABLE QString getConfigFilePath() const { return Setting::GetInstance().getConfigFilePath(); }

    // ---- JSON 导入导出 ----
    Q_INVOKABLE bool exportConfigToJsonFile(const QString &filePath) {
        return Setting::GetInstance().exportConfigToJsonFile(filePath);
    }
    Q_INVOKABLE bool importConfigFromJsonFile(const QString &filePath, bool replaceAll) {
        return Setting::GetInstance().importConfigFromJsonFile(filePath, replaceAll);
    }
    Q_INVOKABLE bool exportDatabaseToJsonFile(const QString &filePath) {
        return Setting::GetInstance().exportDatabaseToJsonFile(filePath);
    }
    Q_INVOKABLE bool importDatabaseFromJsonFile(const QString &filePath, bool replaceAll) {
        return Setting::GetInstance().importDatabaseFromJsonFile(filePath, replaceAll);
    }

    // ---- 代理配置 ----
    Q_INVOKABLE void setProxyType(int type) { Setting::GetInstance().setProxyType(type); }
    Q_INVOKABLE int getProxyType() const { return Setting::GetInstance().getProxyType(); }
    Q_INVOKABLE void setProxyHost(const QString &host) { Setting::GetInstance().setProxyHost(host); }
    Q_INVOKABLE QString getProxyHost() const { return Setting::GetInstance().getProxyHost(); }
    Q_INVOKABLE void setProxyPort(int port) { Setting::GetInstance().setProxyPort(port); }
    Q_INVOKABLE int getProxyPort() const { return Setting::GetInstance().getProxyPort(); }
    Q_INVOKABLE void setProxyUsername(const QString &username) { Setting::GetInstance().setProxyUsername(username); }
    Q_INVOKABLE QString getProxyUsername() const { return Setting::GetInstance().getProxyUsername(); }
    Q_INVOKABLE void setProxyPassword(const QString &password) { Setting::GetInstance().setProxyPassword(password); }
    Q_INVOKABLE QString getProxyPassword() const { return Setting::GetInstance().getProxyPassword(); }
    Q_INVOKABLE void setProxyEnabled(bool enabled) { Setting::GetInstance().setProxyEnabled(enabled); }
    Q_INVOKABLE bool getProxyEnabled() const { return Setting::GetInstance().getProxyEnabled(); }
    Q_INVOKABLE void setProxyConfig(bool enableProxy, int type, const QString &host, int port, const QString &username,
                                    const QString &password) {
        Setting::GetInstance().setProxyConfig(enableProxy, type, host, port, username, password);
    }

    // ---- 服务器配置 ----
    Q_INVOKABLE bool isHttpsUrl(const QString &url) const { return Setting::GetInstance().isHttpsUrl(url); }
    Q_INVOKABLE void updateServerConfig(const QString &baseUrl) { Setting::GetInstance().updateServerConfig(baseUrl); }

  signals:
    void baseUrlChanged();
};
