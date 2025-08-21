#include "config.h"
#include "default_value.h"

#if defined(Q_OS_WIN)
#include <QProcess>
#include <windows.h>
#endif

// 定义布尔类型的设置键名集合
const QSet<QString> Config::booleanKeys = DefaultValues::booleanKeys;

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
    if (!m_config)
        return false;
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
    if (!m_config)
        return defaultValue;
    QVariant value = m_config->value(key, defaultValue);

    if (booleanKeys.contains(key)) {
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