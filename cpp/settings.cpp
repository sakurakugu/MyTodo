#include "settings.h"
#include <QDebug>

Settings::Settings(QObject *parent, StorageType storageType)
    : QObject(parent), m_storageType(storageType) {
    if (m_storageType == IniFile) {
        // 使用INI文件存储
        m_settings = new QSettings("config.ini", QSettings::IniFormat, this);
        qDebug() << "配置已初始化为INI文件模式，位置:" << m_settings->fileName();
    } else {
        // 使用注册表存储
        m_settings = new QSettings("MyTodo", "TodoApp", this);
        qDebug() << "配置已初始化为注册表模式，位置:" << m_settings->fileName();
    }
}

Settings::~Settings() {
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
bool Settings::save(const QString &key, const QVariant &value) {
    if (!m_settings) return false;
    m_settings->setValue(key, value);
    return m_settings->status() == QSettings::NoError;
}

/**
 * @brief 从配置文件读取设置
 * @param key 设置键名
 * @param defaultValue 默认值（如果设置不存在）
 * @return 设置值
 */
QVariant Settings::get(const QString &key, const QVariant &defaultValue) {
    if (!m_settings) return defaultValue;
    return m_settings->value(key, defaultValue);
}

/**
 * @brief 移除设置
 * @param key 设置键名
 */
void Settings::remove(const QString &key) {
    if (!m_settings) return;
    m_settings->remove(key);
}

/**
 * @brief 检查设置是否存在
 * @param key 设置键名
 * @return 设置是否存在
 */
bool Settings::contains(const QString &key) {
    if (!m_settings) return false;
    return m_settings->contains(key);
}

/**
 * @brief 获取所有设置的键名
 * @return 所有设置的键名列表
 */
QStringList Settings::allKeys() {
    if (!m_settings) return QStringList();
    return m_settings->allKeys();
}

/**
 * @brief 清除所有设置
 */
void Settings::clearSettings() {
    if (!m_settings) return;
    m_settings->clear();
}

/**
 * @brief 初始化默认服务器配置
 * 如果服务器配置不存在，则设置默认值
 */
void Settings::initializeDefaultServerConfig() {
    // 检查并设置默认的服务器基础URL
    if (!contains("server/baseUrl")) {
        save("server/baseUrl", "https://api.example.com");
        // qDebug() << "设置默认服务器基础URL: https://api.example.com";
    }
    
    // 检查并设置默认的待办事项API端点
    if (!contains("server/todoApiEndpoint")) {
        save("server/todoApiEndpoint", "/todo_api.php");
        // qDebug() << "设置默认待办事项API端点: /todo_api.php";
    }
    
    // 检查并设置默认的认证API端点
    if (!contains("server/authApiEndpoint")) {
        save("server/authApiEndpoint", "/auth_api.php");
        // qDebug() << "设置默认认证API端点: /auth_api.php";
    }
    
    // qDebug() << "服务器配置初始化完成";
}

/**
 * @brief 获取当前存储类型
 * @return 当前使用的存储类型
 */
Settings::StorageType Settings::getStorageType() const {
    return m_storageType;
}