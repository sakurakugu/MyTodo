/**
 * @brief 构造函数
 *
 * 初始化Setting对象，设置父对象为parent。
 *
 * @param parent 父对象指针，默认值为nullptr。
 * @date 2025-08-21 21:31:41(UTC+8) 周四
 * @change 2025-09-24 00:55:58(UTC+8) 周三
 */
#include "setting.h"
#include "default_value.h"
#include "foundation/config.h"
#include "foundation/database.h"
#include "foundation/network_proxy.h"
#include "foundation/network_request.h"

#include <QFile>
#include <QSaveFile>
#include <string>

Setting::Setting(QObject *parent)
    : QObject(parent),                 //
      m_logger(Logger::GetInstance()), //
      m_config(Config::GetInstance())  //
{
    initializeDefaultServerConfig();
}
Setting::~Setting() {}

int Setting::getOsType() const {
#if defined(Q_OS_WIN)
    return 0;
#else
    return 1;
#endif
}

// 本地配置相关方法实现
void Setting::save(const QString &key, const QVariant &value) {
    m_config.save(key, value);
}

QVariant Setting::get(const QString &key, const QVariant &defaultValue) const {
    return m_config.get(key, defaultValue);
}

void Setting::remove(const QString &key) {
    m_config.remove(key);
}

bool Setting::contains(const QString &key) const {
    return m_config.contains(key);
}

void Setting::clear() {
    m_config.clear();
}

bool Setting::openConfigFilePath() const {
    auto result = m_config.openConfigFilePath();
    return result;
}

QString Setting::getConfigFilePath() const {
    return m_config.getConfigFilePath();
}

bool Setting::exportConfigToJsonFile(const QString &filePath) {
    QStringList excludeKeys;
    excludeKeys << "proxy";
    std::string jsonString = m_config.exportToJson(excludeKeys);
    QSaveFile out(filePath);
    if (!out.open(QIODevice::WriteOnly)) {
        qCritical() << "保存配置JSON失败:" << filePath << out.errorString();
        return false;
    }
    out.write(QByteArray::fromStdString(jsonString));
    if (!out.commit()) {
        qCritical() << "提交保存配置JSON失败:" << filePath << out.errorString();
        return false;
    }
    return true;
}

bool Setting::importConfigFromJsonFile(const QString &filePath, bool replaceAll) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qCritical() << "无法打开配置JSON文件:" << filePath << f.errorString();
        return false;
    }
    QByteArray data = f.readAll();
    f.close();
    return m_config.importFromJson(data.toStdString(), replaceAll);
}

bool Setting::exportDatabaseToJsonFile(const QString &filePath) {
    return Database::GetInstance().exportDatabaseToJsonFile(filePath);
}

bool Setting::importDatabaseFromJsonFile(const QString &filePath, bool replaceAll) {
    return Database::GetInstance().importDatabaseFromJsonFile(filePath, replaceAll);
}

// 日志配置相关方法实现
void Setting::setLogLevel(LogLevel logLevel) {
    m_config.save("log/level", static_cast<int>(logLevel));
    auto result = m_logger.setLogLevel(logLevel);
    if (!result) {
        qWarning() << "无法设置日志级别:" << static_cast<int>(result.error());
    }
}

LogLevel Setting::getLogLevel() const {
    auto result = m_config.get("log/level", static_cast<int>(LogLevel::Info));
    return static_cast<LogLevel>(result.toInt());
}

void Setting::setLogToFile(bool enabled) {
    m_config.save("log/toFile", enabled);
    auto result = m_logger.setLogToFile(enabled);
    if (!result) {
        qWarning() << "无法设置日志是否记录到文件:" << static_cast<int>(result.error());
    }
}

bool Setting::getLogToFile() const {
    auto result = m_config.get("log/toFile", true);
    return result.toBool();
}

void Setting::setLogToConsole(bool enabled) {
    m_config.save("log/toConsole", enabled);
    auto result = m_logger.setLogToConsole(enabled);
    if (!result) {
        qWarning() << "无法设置日志是否记录到控制台:" << static_cast<int>(result.error());
    }
}

bool Setting::getLogToConsole() const {
    auto result = m_config.get("log/toConsole", true);
    return result.toBool();
}

void Setting::setMaxLogFileSize(qint64 maxSize) {
    m_config.save("log/maxFileSize", maxSize);
    auto result = m_logger.setMaxLogFileSize(maxSize);
    if (!result) {
        qWarning() << "无法设置最大日志文件大小:" << static_cast<int>(result.error());
    }
}

qint64 Setting::getMaxLogFileSize() const {
    constexpr qint64 defaultSize = 10 * 1024 * 1024; // 默认10MB
    auto result = m_config.get("log/maxFileSize", defaultSize);
    return result.toLongLong();
}

void Setting::setMaxLogFiles(int maxFiles) {
    m_config.save("log/maxFiles", maxFiles);
    auto result = m_logger.setMaxLogFiles(maxFiles);
    if (!result) {
        qWarning() << "无法设置最大日志文件数量:" << static_cast<int>(result.error());
    }
}

int Setting::getMaxLogFiles() const {
    constexpr int defaultFiles = 5; // 默认保留5个文件
    auto result = m_config.get("log/maxFiles", defaultFiles);
    return result.toInt();
}

QString Setting::getLogFilePath() const {
    return QString::fromStdString(m_logger.getLogFilePath());
}

void Setting::clearLogs() {
    auto result = m_logger.clearLogs();
    if (!result) {
        qWarning() << "无法清除日志:" << static_cast<int>(result.error());
    }
}

/**
 * @brief 初始化默认服务器配置
 * 如果服务器配置不存在，则设置默认值
 */
void Setting::initializeDefaultServerConfig() {
    // 检查并设置默认的服务器基础URL
    if (!m_config.contains("server/baseUrl")) {
        m_config.save("server/baseUrl", QString(DefaultValues::baseUrl));
        NetworkRequest::GetInstance().setServerConfig(QString(DefaultValues::baseUrl));
    } else {
        // 如果已经存在配置，确保NetworkRequest也同步更新
        QString existingUrl = m_config.get("server/baseUrl").toString();
        NetworkRequest::GetInstance().setServerConfig(existingUrl);
    }

    // 检查并设置默认的待办事项API端点
    if (!m_config.contains("server/todoApiEndpoint")) {
        m_config.save("server/todoApiEndpoint", QString(DefaultValues::todoApiEndpoint));
    }

    // 检查并设置默认的认证API端点
    if (!m_config.contains("server/authApiEndpoint")) {
        m_config.save("server/authApiEndpoint", QString(DefaultValues::userAuthApiEndpoint));
    }

    // 检查并设置默认的分类API端点
    if (!m_config.contains("server/categoriesApiEndpoint")) {
        m_config.save("server/categoriesApiEndpoint", QString(DefaultValues::categoriesApiEndpoint));
    }
}

// 代理配置相关方法实现
void Setting::setProxyType(int type) {
    m_config.save("proxy/type", type);
}

int Setting::getProxyType() const {
    auto result = m_config.get("proxy/type", 0); // 默认为NoProxy
    return result.toInt();
}

void Setting::setProxyHost(const QString &host) {
    m_config.save("proxy/host", host);
}

QString Setting::getProxyHost() const {
    auto result = m_config.get("proxy/host", QString());
    return result.toString();
}

void Setting::setProxyPort(int port) {
    m_config.save("proxy/port", port);
}

int Setting::getProxyPort() const {
    auto result = m_config.get("proxy/port", 8080); // 默认端口8080
    return result.toInt();
}

void Setting::setProxyUsername(const QString &username) {
    m_config.save("proxy/username", username);
}

QString Setting::getProxyUsername() const {
    auto result = m_config.get("proxy/username", QString());
    return result.toString();
}

void Setting::setProxyPassword(const QString &password) {
    m_config.save("proxy/password", password);
}

QString Setting::getProxyPassword() const {
    auto result = m_config.get("proxy/password", QString());
    return result.toString();
}

void Setting::setProxyEnabled(bool enabled) {
    m_config.save("proxy/enabled", enabled);
}

bool Setting::getProxyEnabled() const {
    auto result = m_config.get("proxy/enabled", false);
    return result.toBool();
}

/**
 * @brief 检查URL是否使用HTTPS协议
 * @param url 要检查的URL
 * @return 如果使用HTTPS则返回true，否则返回false
 */
bool Setting::isHttpsUrl(const QString &url) const {
    return url.startsWith("https://", Qt::CaseInsensitive);
}

/**
 * @brief 更新服务器配置
 * @param baseUrl 新的服务器基础URL
 */
void Setting::updateServerConfig(const QString &baseUrl) {
    if (baseUrl.isEmpty()) {
        qWarning() << "尝试设置空的服务器URL";
        return;
    }

    NetworkRequest::GetInstance().setServerConfig(baseUrl);
    m_config.save("server/baseUrl", baseUrl);

    qDebug() << "服务器配置已更新:" << baseUrl;
    qDebug() << "HTTPS状态:" << (isHttpsUrl(baseUrl) ? "安全" : "不安全");

    // 发出信号通知其他组件
    emit baseUrlChanged();
}

void Setting::setProxyConfig(bool enableProxy, int type, const QString &host, //
                             int port, const QString &username, const QString &password) {
    NetworkProxy::GetInstance().setProxyConfig(enableProxy, static_cast<NetworkProxy::ProxyType>(type), //
                                               host, port, username, password);
}
