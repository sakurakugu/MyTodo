/**
 * @file network_proxy.cpp
 * @brief 网络代理类实现
 * @author Sakurakugu
 * @date 2025-08-22 23:04:19(UTC+8) 周五
 * @version 2025-08-22 23:04:19(UTC+8) 周五
 */
#include "network_proxy.h"
#include "config.h"

#include <QDebug>
#include <QNetworkProxy>

/**
 * @brief 构造函数
 * @param parent 父对象
 */
NetworkProxy::NetworkProxy(QObject *parent)
    : QObject(parent), m_proxyType(NoProxy), m_proxyPort(0), m_proxyEnabled(false) {
    // 从配置加载代理设置
    loadProxyConfigFromSettings();
}

/**
 * @brief 析构函数
 */
NetworkProxy::~NetworkProxy() {
    // 保存当前配置到设置
    saveProxyConfigToSettings();
}

/**
 * @brief 设置代理配置
 * @param type 代理类型
 * @param host 代理主机地址
 * @param port 代理端口
 * @param username 代理用户名（可选）
 * @param password 代理密码（可选）
 */
void NetworkProxy::setProxyConfig(ProxyType type, const QString &host, int port, const QString &username,
                                  const QString &password) {
    updateProxyConfig(type, host, port, username, password);
    m_proxyEnabled = (type != NoProxy);

    qDebug() << "代理配置已更新" << getProxyDescription();
    emit proxyConfigChanged(type, host, port);
}

/**
 * @brief 将代理配置应用到网络管理器
 * @param manager 网络访问管理器
 */
void NetworkProxy::applyProxyToManager(QNetworkAccessManager *manager) {
    if (!manager) {
        qWarning() << "NetworkAccessManager 为空，无法应用代理配置";
        return;
    }

    QNetworkProxy proxy = createQNetworkProxy();
    manager->setProxy(proxy);

    qDebug() << "代理配置已应用" << getProxyDescription();
}

/**
 * @brief 清除代理配置
 */
void NetworkProxy::clearProxyConfig() {
    setProxyConfig(NoProxy);
}

/**
 * @brief 获取当前代理类型
 * @return 代理类型
 */
NetworkProxy::ProxyType NetworkProxy::getProxyType() const {
    return m_proxyType;
}

/**
 * @brief 获取代理主机地址
 * @return 代理主机地址
 */
QString NetworkProxy::getProxyHost() const {
    return m_proxyHost;
}

/**
 * @brief 获取代理端口
 * @return 代理端口
 */
int NetworkProxy::getProxyPort() const {
    return m_proxyPort;
}

/**
 * @brief 获取代理用户名
 * @return 代理用户名
 */
QString NetworkProxy::getProxyUsername() const {
    return m_proxyUsername;
}

/**
 * @brief 检查是否配置了代理认证
 * @return 如果配置了用户名则返回true
 */
bool NetworkProxy::hasProxyAuth() const {
    return !m_proxyUsername.isEmpty();
}

/**
 * @brief 从配置文件加载代理设置
 */
void NetworkProxy::loadProxyConfigFromSettings() {
    Config &config = Config::GetInstance();

    // 检查是否启用代理
    auto enabledResult = config.get(QStringLiteral("proxy/enabled"), false);
    m_proxyEnabled = enabledResult.has_value() ? enabledResult.value().toBool() : false;

    if (!m_proxyEnabled) {
        updateProxyConfig(NoProxy, QString(), 0, QString(), QString());
        return;
    }

    // 获取代理配置
    auto typeResult = config.get(QStringLiteral("proxy/type"), 0);
    auto hostResult = config.get(QStringLiteral("proxy/host"), QString());
    auto portResult = config.get(QStringLiteral("proxy/port"), 8080);
    auto usernameResult = config.get(QStringLiteral("proxy/username"), QString());
    auto passwordResult = config.get(QStringLiteral("proxy/password"), QString());

    ProxyType type = static_cast<ProxyType>(typeResult.has_value() ? typeResult.value().toInt() : 0);
    QString host = hostResult.has_value() ? hostResult.value().toString() : QString();
    int port = portResult.has_value() ? portResult.value().toInt() : 8080;
    QString username = usernameResult.has_value() ? usernameResult.value().toString() : QString();
    QString password = passwordResult.has_value() ? passwordResult.value().toString() : QString();

    // 应用代理配置
    updateProxyConfig(type, host, port, username, password);

    qDebug() << "已从配置加载代理设置" << getProxyDescription();
}

/**
 * @brief 保存代理设置到配置文件
 */
void NetworkProxy::saveProxyConfigToSettings() {
    Config &config = Config::GetInstance();

    config.save(QStringLiteral("proxy/enabled"), m_proxyEnabled);
    config.save(QStringLiteral("proxy/type"), static_cast<int>(m_proxyType));
    config.save(QStringLiteral("proxy/host"), m_proxyHost);
    config.save(QStringLiteral("proxy/port"), m_proxyPort);
    config.save(QStringLiteral("proxy/username"), m_proxyUsername);
    config.save(QStringLiteral("proxy/password"), m_proxyPassword);

    qDebug() << "代理设置已保存到配置文件";
}

/**
 * @brief 检查代理是否启用
 * @return 如果代理启用则返回true
 */
bool NetworkProxy::isProxyEnabled() const {
    return m_proxyEnabled && (m_proxyType != NoProxy);
}

/**
 * @brief 获取代理配置描述
 * @return 代理配置的文本描述
 */
QString NetworkProxy::getProxyDescription() const {
    if (!isProxyEnabled()) {
        return QStringLiteral("未启用代理");
    }

    QString typeStr;
    switch (m_proxyType) {
    case NoProxy:
        typeStr = QStringLiteral("无代理");
        break;
    case SystemProxy:
        typeStr = QStringLiteral("系统代理");
        break;
    case HttpProxy:
        typeStr = QStringLiteral("HTTP代理");
        break;
    case Socks5Proxy:
        typeStr = QStringLiteral("SOCKS5代理");
        break;
    }

    if (m_proxyType == SystemProxy) {
        return typeStr;
    }

    QString description = QString("%1 - %2:%3").arg(typeStr, m_proxyHost).arg(m_proxyPort);
    if (hasProxyAuth()) {
        description += QString(" (用户: %1)").arg(m_proxyUsername);
    }

    return description;
}

/**
 * @brief 创建QNetworkProxy对象
 * @return 配置好的QNetworkProxy对象
 */
QNetworkProxy NetworkProxy::createQNetworkProxy() const {
    QNetworkProxy proxy;

    switch (m_proxyType) {
    case NoProxy:
        proxy.setType(QNetworkProxy::NoProxy);
        break;
    case SystemProxy:
        proxy.setType(QNetworkProxy::DefaultProxy);
        break;
    case HttpProxy:
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(m_proxyHost);
        proxy.setPort(m_proxyPort);
        if (!m_proxyUsername.isEmpty()) {
            proxy.setUser(m_proxyUsername);
            proxy.setPassword(m_proxyPassword);
        }
        break;
    case Socks5Proxy:
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(m_proxyHost);
        proxy.setPort(m_proxyPort);
        if (!m_proxyUsername.isEmpty()) {
            proxy.setUser(m_proxyUsername);
            proxy.setPassword(m_proxyPassword);
        }
        break;
    }

    return proxy;
}

/**
 * @brief 更新代理配置
 * @param type 代理类型
 * @param host 代理主机
 * @param port 代理端口
 * @param username 用户名
 * @param password 密码
 */
void NetworkProxy::updateProxyConfig(ProxyType type, const QString &host, int port, const QString &username,
                                     const QString &password) {
    m_proxyType = type;
    m_proxyHost = host;
    m_proxyPort = port;
    m_proxyUsername = username;
    m_proxyPassword = password;
}