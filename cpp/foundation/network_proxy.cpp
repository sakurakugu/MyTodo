/**
 * @file network_proxy.cpp
 * @brief 网络代理类实现
 * @author Sakurakugu
 * @date 2025-08-22 23:04:19(UTC+8) 周五
 * @change 2025-08-31 15:07:38(UTC+8) 周日
 * @version 0.4.0
 */
#include "network_proxy.h"
#include "config.h"

/**
 * @brief 构造函数
 * @param parent 父对象
 */
NetworkProxy::NetworkProxy(QObject *parent)
    : QObject(parent), m_proxyType(ProxyType::NoProxy), m_proxyPort(0), m_proxyEnabled(false) {
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
void NetworkProxy::setProxyConfig(bool enableProxy, ProxyType type, const QString &host, int port,
                                  const QString &username, const QString &password) {
    m_proxyEnabled = enableProxy;
    updateProxyConfig(type, host, port, username, password);

    qDebug() << "代理配置已更新" << getProxyDescription();
    emit proxyConfigChanged(type, host, port);
}

/**
 * @brief 将代理配置应用到网络管理器
 * @param manager 网络访问管理器
 */
void NetworkProxy::applyProxyToManager(QNetworkAccessManager *manager) noexcept {
    if (!manager) {
        qWarning() << "NetworkAccessManager 为空，无法应用代理配置";
        return;
    }

    try {
        QNetworkProxy proxy = createQNetworkProxy();
        manager->setProxy(proxy);
        qInfo() << "代理配置为" << getProxyDescription();
    } catch (const std::exception &e) {
        qWarning() << "应用代理配置时发生异常" << ": " << e.what();
    } catch (...) {
        qWarning() << "应用代理配置时发生未知异常";
    }
}

/**
 * @brief 清除代理配置
 */
void NetworkProxy::clearProxyConfig() noexcept {
    setProxyConfig(false, ProxyType::NoProxy);
}

/**
 * @brief 获取当前代理类型
 * @return 代理类型
 */
constexpr NetworkProxy::ProxyType NetworkProxy::getProxyType() const noexcept {
    return m_proxyType;
}

/**
 * @brief 获取代理主机地址
 * @return 代理主机地址
 */
const QString &NetworkProxy::getProxyHost() const noexcept {
    return m_proxyHost;
}

/**
 * @brief 获取代理端口
 * @return 代理端口
 */
constexpr int NetworkProxy::getProxyPort() const noexcept {
    return m_proxyPort;
}

/**
 * @brief 获取代理用户名
 * @return 代理用户名
 */
const QString &NetworkProxy::getProxyUsername() const noexcept {
    return m_proxyUsername;
}

/**
 * @brief 检查是否配置了代理认证
 * @return 如果配置了用户名则返回true
 */
constexpr bool NetworkProxy::hasProxyAuth() const noexcept {
    return !m_proxyUsername.isEmpty();
}

/**
 * @brief 从配置文件加载代理设置
 */
void NetworkProxy::loadProxyConfigFromSettings() noexcept {
    try {
        Config &config = Config::GetInstance();

        // 检查是否启用代理
        auto enabledResult = config.get("proxy/enabled", false);
        m_proxyEnabled = enabledResult.isValid() ? enabledResult.toBool() : false;

        if (!m_proxyEnabled) {
            updateProxyConfig(ProxyType::NoProxy, QString{}, 0, QString{}, QString{});
            return;
        }

        // 获取代理配置
        auto typeResult = config.get("proxy/type", 0);
        auto hostResult = config.get("proxy/host", QString());
        auto portResult = config.get("proxy/port", 8080);
        auto usernameResult = config.get("proxy/username", QString());
        auto passwordResult = config.get("proxy/password", QString());

        ProxyType type = static_cast<ProxyType>(typeResult.isValid() ? typeResult.toInt() : 0);
        QString host = hostResult.isValid() ? hostResult.toString() : QString();
        int port = portResult.isValid() ? portResult.toInt() : 8080;
        QString username = usernameResult.isValid() ? usernameResult.toString() : QString();
        QString password = passwordResult.isValid() ? passwordResult.toString() : QString();

        // 应用代理配置
        updateProxyConfig(type, host, port, username, password);

        qDebug() << "已从配置加载代理设置" << getProxyDescription();
        return;
    } catch (const std::exception &e) {
        qWarning() << "加载代理配置时发生异常" << ": " << e.what();
        return;
    } catch (...) {
        qWarning() << "加载代理配置时发生未知异常";
        return;
    }
}

/**
 * @brief 保存代理设置到配置文件
 */
void NetworkProxy::saveProxyConfigToSettings() noexcept {
    try {
        Config &config = Config::GetInstance();

        config.save("proxy/enabled", m_proxyEnabled);
        config.save("proxy/type", static_cast<int>(m_proxyType));
        config.save("proxy/host", m_proxyHost);
        config.save("proxy/port", m_proxyPort);
        config.save("proxy/username", m_proxyUsername);
        config.save("proxy/password", m_proxyPassword);

        qDebug() << "代理设置已保存到配置文件";
        return;
    } catch (const std::exception &e) {
        qWarning() << "保存代理配置时发生异常" << ": " << e.what();
        return;
    } catch (...) {
        qWarning() << "保存代理配置时发生未知异常";
        return;
    }
}

/**
 * @brief 获取代理配置描述
 * @return 代理配置的文本描述
 */
QString NetworkProxy::getProxyDescription() const noexcept {
    if (!m_proxyEnabled) {
        return QStringLiteral("未启用");
    }

    constexpr auto getTypeString = [](ProxyType type) -> std::string_view {
        switch (type) {
        case ProxyType::NoProxy:
            return "无代理";
        case ProxyType::SystemProxy:
            return "系统代理";
        case ProxyType::HttpProxy:
            return "HTTP代理";
        case ProxyType::Socks5Proxy:
            return "SOCKS5代理";
        case ProxyType::HybridProxy:
            return "混合代理";
        }
        return "未知代理类型";
    };

    const QString typeStr = QString::fromUtf8(getTypeString(m_proxyType));

    if (m_proxyType == ProxyType::NoProxy || m_proxyType == ProxyType::SystemProxy) {
        return typeStr;
    }

    QString description = QStringLiteral("%1 - %2:%3").arg(typeStr, m_proxyHost).arg(m_proxyPort);
    if (hasProxyAuth()) {
        description += QStringLiteral(" (用户: %1)").arg(m_proxyUsername);
    }

    return description;
}

/**
 * @brief 创建QNetworkProxy对象
 * @return 配置好的QNetworkProxy对象
 */
QNetworkProxy NetworkProxy::createQNetworkProxy() const noexcept {
    QNetworkProxy proxy;

    switch (m_proxyType) {
    case ProxyType::NoProxy:
        proxy.setType(QNetworkProxy::NoProxy);
        break;
    case ProxyType::SystemProxy:
        proxy.setType(QNetworkProxy::DefaultProxy);
        break;
    case ProxyType::HybridProxy:
        [[fallthrough]];
    case ProxyType::HttpProxy:
        [[fallthrough]];
    case ProxyType::Socks5Proxy: {
        const auto proxyType =
            (m_proxyType == ProxyType::HttpProxy) ? QNetworkProxy::HttpProxy : QNetworkProxy::Socks5Proxy;
        proxy.setType(proxyType);
        proxy.setHostName(m_proxyHost);
        proxy.setPort(static_cast<quint16>(m_proxyPort));

        if (hasProxyAuth()) {
            proxy.setUser(m_proxyUsername);
            proxy.setPassword(m_proxyPassword);
        }
        break;
    }
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
                                     const QString &password) noexcept {
    m_proxyType = type;
    m_proxyHost = host;
    m_proxyPort = port;
    m_proxyUsername = username;
    m_proxyPassword = password;
}