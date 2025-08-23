/**
 * @file network_proxy.h
 * @brief NetworkProxy类的头文件
 *
 * 该文件定义了NetworkProxy类，用于管理应用程序的网络代理配置。
 *
 * @author Sakurakugu
 * @date 2025-08-22 23:04:19(UTC+8) 周五
 * @version 2025-08-22 23:04:19(UTC+8) 周五
 */

#pragma once

#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QObject>
#include <QString>

/**
 * @class NetworkProxy
 * @brief 网络代理管理器，提供统一的代理配置和管理功能
 *
 * NetworkProxy类是应用程序网络代理层的核心组件，提供了完整的代理管理功能：
 *
 * **核心功能：**
 * - 代理类型配置和切换
 * - 代理服务器连接参数设置
 * - 代理认证信息管理
 * - 从配置文件加载代理设置
 *
 * **支持的代理类型：**
 * - 无代理（直连）
 * - 系统代理
 * - HTTP代理
 * - SOCKS5代理
 *
 * **使用场景：**
 * - 企业网络环境下的代理配置
 * - 网络访问控制和安全管理
 * - 多环境代理切换
 * - 代理服务器负载均衡
 *
 * @note 所有代理配置操作都会立即应用到指定的QNetworkAccessManager
 * @see NetworkRequest
 */
class NetworkProxy : public QObject {
    Q_OBJECT

  public:
    /**
     * @enum ProxyType
     * @brief 代理类型枚举
     */
    enum ProxyType {
        NoProxy,        // 不使用代理
        SystemProxy,    // 使用系统代理
        HttpProxy,      // HTTP代理
        Socks5Proxy     // SOCKS5代理
    };
    Q_ENUM(ProxyType)

    // 构造函数和析构函数
    explicit NetworkProxy(QObject *parent = nullptr);
    ~NetworkProxy();

    // 代理配置管理
    void setProxyConfig(ProxyType type, const QString &host = QString(), int port = 0, 
                       const QString &username = QString(), const QString &password = QString()); // 设置代理配置
    void applyProxyToManager(QNetworkAccessManager *manager); // 将代理配置应用到网络管理器
    void clearProxyConfig(); // 清除代理配置

    // 代理信息获取
    ProxyType getProxyType() const; // 获取当前代理类型
    QString getProxyHost() const;   // 获取代理主机
    int getProxyPort() const;       // 获取代理端口
    QString getProxyUsername() const; // 获取代理用户名
    bool hasProxyAuth() const;      // 检查是否配置了代理认证

    // 配置文件操作
    void loadProxyConfigFromSettings(); // 从配置加载代理设置
    void saveProxyConfigToSettings();   // 保存代理设置到配置

    // 代理状态检查
    bool isProxyEnabled() const;    // 检查代理是否启用
    QString getProxyDescription() const; // 获取代理配置描述

  signals:
    void proxyConfigChanged(ProxyType type, const QString &host, int port); // 代理配置改变信号

  private:
    // 内部方法
    QNetworkProxy createQNetworkProxy() const; // 创建QNetworkProxy对象
    void updateProxyConfig(ProxyType type, const QString &host, int port, 
                          const QString &username, const QString &password); // 更新代理配置

    // 成员变量
    ProxyType m_proxyType;     // 代理类型
    QString m_proxyHost;       // 代理主机
    int m_proxyPort;           // 代理端口
    QString m_proxyUsername;   // 代理用户名
    QString m_proxyPassword;   // 代理密码
    bool m_proxyEnabled;       // 代理是否启用
};