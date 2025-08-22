#include "setting.h"

Setting::Setting(QObject *parent) : QObject(parent), m_logger(Logger::GetInstance()), m_config(Config::GetInstance()) {
}

Setting::~Setting() {
}

int Setting::getOsType() const {
#if defined(Q_OS_WIN)
    return 0;
#else
    return 1;
#endif
}

// 本地配置相关方法实现
bool Setting::save(const QString &key, const QVariant &value) {
    auto result = m_config.save(key, value);
    return result.has_value();
}

QVariant Setting::get(const QString &key, const QVariant &defaultValue) const {
    auto result = m_config.get(key, defaultValue);
    return result.has_value() ? result.value() : defaultValue;
}

void Setting::remove(const QString &key) {
    m_config.remove(key);
}

bool Setting::contains(const QString &key) {
    return m_config.contains(key);
}

QStringList Setting::allKeys() {
    return m_config.allKeys();
}

void Setting::clear() {
    auto result = m_config.clear();
    if (!result) {
        // 记录错误但不抛出异常，保持向后兼容
        qWarning() << "无法清除设置";
    }
}

// 存储类型和路径管理相关方法实现
bool Setting::openConfigFilePath() const {
    auto result = m_config.openConfigFilePath();
    return result.has_value();
}

QString Setting::getConfigFilePath() const {
    return m_config.getConfigFilePath();
}

// 日志配置相关方法实现
void Setting::setLogLevel(int level) {
    m_config.save(QStringLiteral("log/level"), level);
    auto result = m_logger.setLogLevel(static_cast<Logger::LogLevel>(level));
    if (!result) {
        qWarning() << "无法设置日志级别:" << static_cast<int>(result.error());
    }
}

int Setting::getLogLevel() const {
    auto result = m_config.get(QStringLiteral("log/level"), static_cast<int>(Logger::LogLevel::Info));
    return result.has_value() ? result.value().toInt() : static_cast<int>(Logger::LogLevel::Info);
}

void Setting::setLogToFile(bool enabled) {
    m_config.save(QStringLiteral("log/toFile"), enabled);
    auto result = m_logger.setLogToFile(enabled);
    if (!result) {
        qWarning() << "无法设置日志是否记录到文件:" << static_cast<int>(result.error());
    }
}

bool Setting::getLogToFile() const {
    auto result = m_config.get(QStringLiteral("log/toFile"), true);
    return result.has_value() ? result.value().toBool() : true;
}

void Setting::setLogToConsole(bool enabled) {
    m_config.save(QStringLiteral("log/toConsole"), enabled);
    auto result = m_logger.setLogToConsole(enabled);
    if (!result) {
        qWarning() << "无法设置日志是否记录到控制台:" << static_cast<int>(result.error());
    }
}

bool Setting::getLogToConsole() const {
    auto result = m_config.get(QStringLiteral("log/toConsole"), true);
    return result.has_value() ? result.value().toBool() : true;
}

void Setting::setMaxLogFileSize(qint64 maxSize) {
    m_config.save(QStringLiteral("log/maxFileSize"), maxSize);
    auto result = m_logger.setMaxLogFileSize(maxSize);
    if (!result) {
        qWarning() << "无法设置最大日志文件大小:" << static_cast<int>(result.error());
    }
}

qint64 Setting::getMaxLogFileSize() const {
    constexpr qint64 defaultSize = 10 * 1024 * 1024; // 默认10MB
    auto result = m_config.get(QStringLiteral("log/maxFileSize"), defaultSize);
    return result.has_value() ? result.value().toLongLong() : defaultSize;
}

void Setting::setMaxLogFiles(int maxFiles) {
    m_config.save(QStringLiteral("log/maxFiles"), maxFiles);
    auto result = m_logger.setMaxLogFiles(maxFiles);
    if (!result) {
        qWarning() << "无法设置最大日志文件数量:" << static_cast<int>(result.error());
    }
}

int Setting::getMaxLogFiles() const {
    constexpr int defaultFiles = 5; // 默认保留5个文件
    auto result = m_config.get(QStringLiteral("log/maxFiles"), defaultFiles);
    return result.has_value() ? result.value().toInt() : defaultFiles;
}

QString Setting::getLogFilePath() const {
    return m_logger.getLogFilePath();
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
    if (!m_config.contains(QStringLiteral("server/baseUrl"))) {
        m_config.save(QStringLiteral("server/baseUrl"), QString::fromStdString(std::string{DefaultValues::baseUrl}));
    }

    // 检查并设置默认的待办事项API端点
    if (!m_config.contains(QStringLiteral("server/todoApiEndpoint"))) {
        m_config.save(QStringLiteral("server/todoApiEndpoint"),
                      QString::fromStdString(std::string{DefaultValues::todoApiEndpoint}));
    }

    // 检查并设置默认的认证API端点
    if (!m_config.contains(QStringLiteral("server/authApiEndpoint"))) {
        m_config.save(QStringLiteral("server/authApiEndpoint"),
                      QString::fromStdString(std::string{DefaultValues::userApiEndpoint}));
    }
}

// 代理配置相关方法实现
void Setting::setProxyType(int type) {
    m_config.save(QStringLiteral("proxy/type"), type);
}

int Setting::getProxyType() const {
    auto result = m_config.get(QStringLiteral("proxy/type"), 0); // 默认为NoProxy
    return result.has_value() ? result.value().toInt() : 0;
}

void Setting::setProxyHost(const QString &host) {
    m_config.save(QStringLiteral("proxy/host"), host);
}

QString Setting::getProxyHost() const {
    auto result = m_config.get(QStringLiteral("proxy/host"), QString());
    return result.has_value() ? result.value().toString() : QString();
}

void Setting::setProxyPort(int port) {
    m_config.save(QStringLiteral("proxy/port"), port);
}

int Setting::getProxyPort() const {
    auto result = m_config.get(QStringLiteral("proxy/port"), 8080); // 默认端口8080
    return result.has_value() ? result.value().toInt() : 8080;
}

void Setting::setProxyUsername(const QString &username) {
    m_config.save(QStringLiteral("proxy/username"), username);
}

QString Setting::getProxyUsername() const {
    auto result = m_config.get(QStringLiteral("proxy/username"), QString());
    return result.has_value() ? result.value().toString() : QString();
}

void Setting::setProxyPassword(const QString &password) {
    m_config.save(QStringLiteral("proxy/password"), password);
}

QString Setting::getProxyPassword() const {
    auto result = m_config.get(QStringLiteral("proxy/password"), QString());
    return result.has_value() ? result.value().toString() : QString();
}

void Setting::setProxyEnabled(bool enabled) {
    m_config.save(QStringLiteral("proxy/enabled"), enabled);
}

bool Setting::getProxyEnabled() const {
    auto result = m_config.get(QStringLiteral("proxy/enabled"), false);
    return result.has_value() ? result.value().toBool() : false;
}