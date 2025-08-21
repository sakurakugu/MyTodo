#include "config.h"
#include "logger.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QStandardPaths>
#include <QUrl>
#if defined(Q_OS_WIN)
#include <QProcess>
#include <windows.h>
#endif

Config::Config(QObject *parent) // 构造函数
    : QObject(parent), m_config(nullptr) {
    m_config = new QSettings("MyTodo", "TodoApp", this);
    qDebug() << "配置存放在:" << m_config->fileName();
}

Config::~Config() {
    // 确保所有设置都已保存
    if (m_config) {
        m_config->sync();
    }
}

/**
 * @brief 保存设置到配置文件
 * @param key 设置键名
 * @param value 设置值
 * @return 保存是否成功
 */
bool Config::save(const QString &key, const QVariant &value) {
    // qDebug() << "准备保存设置" << key << "，要保存的值为" << value;
    if (!m_config)
        return false;
    // if (key == "setting/isDarkMode") {
    //     qDebug() << "当前存储的设置值为" << m_config->value(key, value);
    //     qDebug() << "保存设置" << key << "，值为" << value;
    // }
    m_config->setValue(key, value);
    return m_config->status() == QSettings::NoError;
}

/**
 * @brief 从配置文件读取设置
 * @param key 设置键名
 * @param defaultValue 默认值（如果设置不存在）
 * @return 设置值
 */
QVariant Config::get(const QString &key, const QVariant &defaultValue) const {
    // if (key == "setting/isDarkMode") {
    //     qDebug() << "调用get方法, key为" << key << "，当前存储的设置值为" << m_config->value(key, defaultValue);
    // }
    // QSettings存储
    if (!m_config)
        return defaultValue;
    // return m_config->value(key, defaultValue);

    QVariant value = m_config->value(key, defaultValue);

    // 对于布尔类型的设置，确保正确转换字符串
    if (key == "setting/isDarkMode" || key == "setting/preventDragging" || key == "setting/autoSync") {
        if (value.typeId() == QMetaType::QString) {
            QString strValue = value.toString().toLower();
            if (strValue == "true" || strValue == "1") {
                return QVariant(true);
            } else if (strValue == "false" || strValue == "0") {
                return QVariant(false);
            }
        }
    }

    return value;
}

/**
 * @brief 移除设置
 * @param key 设置键名
 */
void Config::remove(const QString &key) {
    if (!m_config)
        return;
    m_config->remove(key);
}

/**
 * @brief 检查设置是否存在
 * @param key 设置键名
 * @return 设置是否存在
 */
bool Config::contains(const QString &key) {
    if (!m_config)
        return false;
    return m_config->contains(key);
}

/**
 * @brief 获取所有设置的键名
 * @return 所有设置的键名列表
 */
QStringList Config::allKeys() {
    if (!m_config)
        return QStringList();
    return m_config->allKeys();
}

/**
 * @brief 清除所有设置
 */
void Config::clearSettings() {
    if (!m_config)
        return;
    m_config->clear();
}

/**
 * @brief 初始化默认服务器配置
 * 如果服务器配置不存在，则设置默认值
 */
void Config::initializeDefaultServerConfig() {
    // 检查并设置默认的服务器基础URL
    if (!contains("server/baseUrl")) {
        save("server/baseUrl", "https://api.example.com");
    }

    // 检查并设置默认的待办事项API端点
    if (!contains("server/todoApiEndpoint")) {
        save("server/todoApiEndpoint", "/todo/todo_api.php");
    }

    // 检查并设置默认的认证API端点
    if (!contains("server/authApiEndpoint")) {
        save("server/authApiEndpoint", "/auth_api.php");
    }
}

// 日志配置相关方法实现
void Config::setLogLevel(int level) {
    save("log/level", level);
    Logger::GetInstance().setLogLevel(static_cast<Logger::LogLevel>(level));
}

int Config::getLogLevel() const {
    return get("log/level", static_cast<int>(Logger::Info)).toInt();
}

void Config::setLogToFile(bool enabled) {
    save("log/toFile", enabled);
    Logger::GetInstance().setLogToFile(enabled);
}

bool Config::getLogToFile() const {
    return get("log/toFile", true).toBool();
}

void Config::setLogToConsole(bool enabled) {
    save("log/toConsole", enabled);
    Logger::GetInstance().setLogToConsole(enabled);
}

bool Config::getLogToConsole() const {
    return get("log/toConsole", true).toBool();
}

void Config::setMaxLogFileSize(qint64 maxSize) {
    save("log/maxFileSize", maxSize);
    Logger::GetInstance().setMaxLogFileSize(maxSize);
}

qint64 Config::getMaxLogFileSize() const {
    return get("log/maxFileSize", 10 * 1024 * 1024).toLongLong(); // 默认10MB
}

void Config::setMaxLogFiles(int maxFiles) {
    save("log/maxFiles", maxFiles);
    Logger::GetInstance().setMaxLogFiles(maxFiles);
}

int Config::getMaxLogFiles() const {
    return get("log/maxFiles", 5).toInt(); // 默认保留5个文件
}

QString Config::getLogFilePath() const {
    return Logger::GetInstance().getLogFilePath();
}

void Config::clearLogs() {
    Logger::GetInstance().clearLogs();
}

/**
 * @brief 获取配置文件路径
 * @return 配置文件路径
 */
QString Config::getConfigFilePath() const {
    return m_config->fileName();
}

/**
 * @brief 打开配置文件路径
 * @return 操作是否成功
 */
bool Config::openConfigFilePath() const {
#if !defined(Q_OS_WIN)
    QString filePath = m_config->fileName();
    if (!filePath.isEmpty()) {
        return QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }
#endif
    return false;
}