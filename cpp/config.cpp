#include "config.h"

#include <QDebug>

Config::Config(QObject *parent, StorageType storageType)  // 构造函数
    : QObject(parent), m_storageType(storageType) {
    // TODO: 在不同平台上的情况
    if (m_storageType == IniFile) {
        // 使用INI文件存储
        m_settings = new QSettings("config.ini", QSettings::IniFormat, this);
        qDebug() << "配置存放在INI文件中，位置在:" << m_settings->fileName();
    } else {
        // 使用注册表存储
        m_settings = new QSettings("MyTodo", "TodoApp", this);
        qDebug() << "配置存放在注册表中，位置在:" << m_settings->fileName();
    }
}

Config::~Config() {
    // 确保所有设置都已保存
    if (m_settings) {
        m_settings->sync();
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
    if (!m_settings) return false;
    // if (key == "setting/isDarkMode") {
    //     qDebug() << "当前存储的设置值为" << m_settings->value(key, value);
    //     qDebug() << "保存设置" << key << "，值为" << value;
    // }
    m_settings->setValue(key, value);
    return m_settings->status() == QSettings::NoError;
}

/**
 * @brief 从配置文件读取设置
 * @param key 设置键名
 * @param defaultValue 默认值（如果设置不存在）
 * @return 设置值
 */
QVariant Config::get(const QString &key, const QVariant &defaultValue) {
    // if (key == "setting/isDarkMode") {
    //     qDebug() << "调用get方法, key为" << key << "，当前存储的设置值为" << m_settings->value(key, defaultValue);
    // }
    if (!m_settings) return defaultValue;
    // return m_settings->value(key, defaultValue);
    
    QVariant value = m_settings->value(key, defaultValue);
    
    // 对于布尔类型的设置，确保正确转换字符串
    if (key == "setting/isDarkMode" || key == "setting/preventDragging" || key == "setting/autoSync") {
        if (value.type() == QVariant::String) {
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
    if (!m_settings) return;
    m_settings->remove(key);
}

/**
 * @brief 检查设置是否存在
 * @param key 设置键名
 * @return 设置是否存在
 */
bool Config::contains(const QString &key) {
    if (!m_settings) return false;
    return m_settings->contains(key);
}

/**
 * @brief 获取所有设置的键名
 * @return 所有设置的键名列表
 */
QStringList Config::allKeys() {
    if (!m_settings) return QStringList();
    return m_settings->allKeys();
}

/**
 * @brief 清除所有设置
 */
void Config::clearSettings() {
    if (!m_settings) return;
    m_settings->clear();
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
        save("server/todoApiEndpoint", "/todo_api.php");
    }

    // 检查并设置默认的认证API端点
    if (!contains("server/authApiEndpoint")) {
        save("server/authApiEndpoint", "/auth_api.php");
    }
}

/**
 * @brief 获取当前存储类型
 * @return 当前使用的存储类型
 */
Config::StorageType Config::getStorageType() const { return m_storageType; }
