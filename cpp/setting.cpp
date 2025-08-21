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
    return m_config.save(key, value);
}

QVariant Setting::get(const QString &key, const QVariant &defaultValue) const {
    return m_config.get(key, defaultValue);
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

void Setting::clearSettings() {
    auto result = m_config.clearSettings();
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
    m_config.save("log/level", level);
    auto result = m_logger.setLogLevel(static_cast<Logger::LogLevel>(level));
    if (!result) {
        qWarning() << "无法设置日志级别:" << static_cast<int>(result.error());
    }
}

int Setting::getLogLevel() const {
    return m_config.get("log/level", static_cast<int>(Logger::LogLevel::Info)).toInt();
}

void Setting::setLogToFile(bool enabled) {
    m_config.save("log/toFile", enabled);
    auto result = m_logger.setLogToFile(enabled);
    if (!result) {
        qWarning() << "无法设置日志是否记录到文件:" << static_cast<int>(result.error());
    }
}

bool Setting::getLogToFile() const {
    return m_config.get("log/toFile", true).toBool();
}

void Setting::setLogToConsole(bool enabled) {
    m_config.save("log/toConsole", enabled);
    auto result = m_logger.setLogToConsole(enabled);
    if (!result) {
        qWarning() << "无法设置日志是否记录到控制台:" << static_cast<int>(result.error());
    }
}

bool Setting::getLogToConsole() const {
    return m_config.get("log/toConsole", true).toBool();
}

void Setting::setMaxLogFileSize(qint64 maxSize) {
    m_config.save("log/maxFileSize", maxSize);
    auto result = m_logger.setMaxLogFileSize(maxSize);
    if (!result) {
        qWarning() << "无法设置最大日志文件大小:" << static_cast<int>(result.error());
    }
}

qint64 Setting::getMaxLogFileSize() const {
    return m_config.get("log/maxFileSize", 10 * 1024 * 1024).toLongLong(); // 默认10MB
}

void Setting::setMaxLogFiles(int maxFiles) {
    m_config.save("log/maxFiles", maxFiles);
    auto result = m_logger.setMaxLogFiles(maxFiles);
    if (!result) {
        qWarning() << "无法设置最大日志文件数量:" << static_cast<int>(result.error());
    }
}

int Setting::getMaxLogFiles() const {
    return m_config.get("log/maxFiles", 5).toInt(); // 默认保留5个文件
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
    if (!m_config.contains("server/baseUrl")) {
        m_config.save("server/baseUrl", QString::fromStdString(std::string{DefaultValues::baseUrl}));
    }

    // 检查并设置默认的待办事项API端点
    if (!m_config.contains("server/todoApiEndpoint")) {
        m_config.save("server/todoApiEndpoint", QString::fromStdString(std::string{DefaultValues::todoApiEndpoint}));
    }

    // 检查并设置默认的认证API端点
    if (!m_config.contains("server/authApiEndpoint")) {
        m_config.save("server/authApiEndpoint", QString::fromStdString(std::string{DefaultValues::userApiEndpoint}));
    }
}