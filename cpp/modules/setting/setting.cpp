/**
 * @file setting.cpp
 * @brief Setting类的实现文件
 *
 * 该文件实现了Setting类，用于管理应用程序的设置和配置。
 *
 * @author Sakurakugu
 * @date 2025-08-21 21:31:41(UTC+8) 周四
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */
#include "setting.h"
#include "backup_manager.h"
#include "config.h"
#include "database.h"
#include "default_value.h"
#include "network_proxy.h"
#include "network_request.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

Setting::Setting(QObject *parent)
    : QObject(parent),                              //
      m_logger(Logger::GetInstance()),              //
      m_config(Config::GetInstance()),              //
      m_backupManager(BackupManager::GetInstance()) //
{
    初始化默认服务器配置();
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
void Setting::保存(const QString &key, const QVariant &value) {
    m_config.save(key, value);
}

QVariant Setting::读取(const QString &key, const QVariant &defaultValue) const {
    return m_config.get(key, defaultValue);
}

void Setting::移除(const QString &key) {
    m_config.remove(key);
}

bool Setting::包含(const QString &key) const {
    return m_config.contains(key);
}

void Setting::清除所有() {
    m_config.clear();
}

bool Setting::打开配置文件路径() const {
    auto result = m_config.openConfigFilePath();
    return result;
}

QString Setting::获取配置文件路径() const {
    return m_config.getConfigFilePath();
}

bool Setting::导出配置到JSON文件(const QString &filePath) {
    std::vector<std::string> excludeKeys = {"proxy"};
    return m_config.exportToJsonFile(filePath.toStdString(), excludeKeys);
}

bool Setting::导入配置从JSON文件(const QString &filePath, bool replaceAll) {
    return m_config.importFromJsonFile(filePath.toStdString(), replaceAll);
}

bool Setting::导出数据库到JSON文件(const QString &filePath) {
    // return Database::GetInstance().exportDatabaseToJsonFile(filePath);
}

bool Setting::导入数据库从JSON文件(const QString &filePath, bool replaceAll) {
    // return Database::GetInstance().importDatabaseFromJsonFile(filePath, replaceAll);
}

// 日志配置相关方法实现
void Setting::设置日志级别(LogLevel logLevel) {
    m_config.save("log/level", static_cast<int>(logLevel));
    auto result = m_logger.setLogLevel(logLevel);
    if (!result) {
        qWarning() << "无法设置日志级别:" << static_cast<int>(result.error());
    }
}

LogLevel Setting::获取日志级别() const {
    auto result = m_config.get("log/level", static_cast<int>(LogLevel::Info));
    return static_cast<LogLevel>(result.toInt());
}

void Setting::设置日志是否记录到文件(bool enabled) {
    m_config.save("log/toFile", enabled);
    auto result = m_logger.setLogToFile(enabled);
    if (!result) {
        qWarning() << "无法设置日志是否记录到文件:" << static_cast<int>(result.error());
    }
}

bool Setting::获取日志是否记录到文件() const {
    auto result = m_config.get("log/toFile", true);
    return result.toBool();
}

void Setting::设置日志是否输出到控制台(bool enabled) {
    m_config.save("log/toConsole", enabled);
    auto result = m_logger.setLogToConsole(enabled);
    if (!result) {
        qWarning() << "无法设置日志是否记录到控制台:" << static_cast<int>(result.error());
    }
}

bool Setting::获取日志是否输出到控制台() const {
    auto result = m_config.get("log/toConsole", true);
    return result.toBool();
}

void Setting::设置最大日志文件大小(qint64 maxSize) {
    m_config.save("log/maxFileSize", maxSize);
    auto result = m_logger.setMaxLogFileSize(maxSize);
    if (!result) {
        qWarning() << "无法设置最大日志文件大小:" << static_cast<int>(result.error());
    }
}

qint64 Setting::获取最大日志文件大小() const {
    constexpr qint64 defaultSize = 10 * 1024 * 1024; // 默认10MB
    auto result = m_config.get("log/maxFileSize", defaultSize);
    return result.toLongLong();
}

void Setting::设置最大日志文件数量(int maxFiles) {
    m_config.save("log/maxFiles", maxFiles);
    auto result = m_logger.setMaxLogFiles(maxFiles);
    if (!result) {
        qWarning() << "无法设置最大日志文件数量:" << static_cast<int>(result.error());
    }
}

int Setting::获取最大日志文件数量() const {
    constexpr int defaultFiles = 5; // 默认保留5个文件
    auto result = m_config.get("log/maxFiles", defaultFiles);
    return result.toInt();
}

QString Setting::获取日志文件路径() const {
    return QString::fromStdString(m_logger.getLogFilePath());
}

void Setting::清除所有日志文件() {
    auto result = m_logger.clearLogs();
    if (!result) {
        qWarning() << "无法清除日志:" << static_cast<int>(result.error());
    }
}

// ---- 配置文件位置管理 ----
int Setting::获取配置文件位置() const {
    return static_cast<int>(m_config.getConfigLocation());
}

QString Setting::获取指定配置位置路径(int location) const {
    Config::Location local = Config::Location::ApplicationPath;
    if (location == static_cast<int>(Config::Location::AppDataRoaming))
        local = Config::Location::AppDataRoaming;

    return QString::fromStdString(m_config.getConfigLocationPath(local));
}

bool Setting::迁移配置文件到位置(int targetLocation, bool overwrite) {
    Config::Location target = Config::Location::ApplicationPath;
    if (targetLocation == static_cast<int>(Config::Location::AppDataRoaming))
        target = Config::Location::AppDataRoaming;

    return m_config.setConfigLocation(target, overwrite);
}

/**
 * @brief 设置自动备份启用状态
 * @param enabled 是否启用自动备份
 */
void Setting::设置自动备份启用状态(bool enabled) {
    m_backupManager.设置自动备份启用状态(enabled);
}

bool Setting::执行备份() {
    return m_backupManager.执行备份();
}

/**
 * @brief 初始化默认服务器配置
 * 如果服务器配置不存在，则设置默认值
 */
void Setting::初始化默认服务器配置() {
    // 检查并设置默认的服务器基础URL
    if (!m_config.contains("server/baseUrl")) {
        m_config.save("server/baseUrl", QString(DefaultValues::baseUrl));
        m_config.save("server/apiVersion", QString(DefaultValues::apiVersion));
        NetworkRequest::GetInstance().setServerConfig(QString(DefaultValues::baseUrl),
                                                      QString(DefaultValues::apiVersion));
    } else {
        // 如果已经存在配置，确保NetworkRequest也同步更新
        QString existingUrl = m_config.get("server/baseUrl").toString();
        QString existingVersion = m_config.get("server/apiVersion", QString(DefaultValues::apiVersion)).toString();
        NetworkRequest::GetInstance().setServerConfig(existingUrl, existingVersion);
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
void Setting::设置代理类型(int type) {
    m_config.save("proxy/type", type);
}

int Setting::获取代理类型() const {
    auto result = m_config.get("proxy/type", 0); // 默认为NoProxy
    return result.toInt();
}

void Setting::设置代理主机(const QString &host) {
    m_config.save("proxy/host", host);
}

QString Setting::获取代理主机() const {
    auto result = m_config.get("proxy/host", QString());
    return result.toString();
}

void Setting::设置代理端口(int port) {
    m_config.save("proxy/port", port);
}

int Setting::获取代理端口() const {
    auto result = m_config.get("proxy/port", 8080); // 默认端口8080
    return result.toInt();
}

void Setting::设置代理用户名(const QString &username) {
    m_config.save("proxy/username", username);
}

QString Setting::获取代理用户名() const {
    auto result = m_config.get("proxy/username", QString());
    return result.toString();
}

void Setting::设置代理密码(const QString &password) {
    m_config.save("proxy/password", password);
}

QString Setting::获取代理密码() const {
    auto result = m_config.get("proxy/password", QString());
    return result.toString();
}

void Setting::设置代理是否启用(bool enabled) {
    m_config.save("proxy/enabled", enabled);
}

bool Setting::获取代理是否启用() const {
    auto result = m_config.get("proxy/enabled", false);
    return result.toBool();
}

/**
 * @brief 检查URL是否使用HTTPS协议
 * @param url 要检查的URL
 * @return 如果使用HTTPS则返回true，否则返回false
 */
bool Setting::是否是HttpsURL(const QString &url) const {
    return url.startsWith("https://", Qt::CaseInsensitive);
}

/**
 * @brief 更新服务器配置
 * @param baseUrl 新的服务器基础URL
 */
void Setting::更新服务器配置(const QString &baseUrl) {
    if (baseUrl.isEmpty()) {
        qWarning() << "尝试设置空的服务器URL";
        return;
    }

    NetworkRequest::GetInstance().setServerConfig(baseUrl);
    m_config.save("server/baseUrl", baseUrl);

    qDebug() << "服务器配置已更新:" << baseUrl;
    qDebug() << "HTTPS状态:" << (是否是HttpsURL(baseUrl) ? "安全" : "不安全");

    // 发出信号通知其他组件
    emit baseUrlChanged();
}

void Setting::设置代理配置(bool enableProxy, int type, const QString &host, //
                           int port, const QString &username, const QString &password) {
    NetworkProxy::GetInstance().setProxyConfig(enableProxy, static_cast<NetworkProxy::ProxyType>(type), //
                                               host, port, username, password);
}

// ---- 自动备份相关方法实现 ----
