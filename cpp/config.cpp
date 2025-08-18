#include "config.h"

#include <QDebug>
#include <QStandardPaths>

Config::Config(QObject *parent, StorageType storageType) // 构造函数
    : QObject(parent), m_storageType(storageType), m_config(nullptr) {
    // TODO: 在不同平台上的情况
    if (m_storageType == IniFile) {
        // 使用INI文件存储
        m_config = new QSettings("config.ini", QSettings::IniFormat, this);
        qDebug() << "配置存放在INI文件中，位置在:" << m_config->fileName();
    } else if (m_storageType == Registry) {
        // 使用注册表存储
        m_config = new QSettings("MyTodo", "TodoApp", this);
        qDebug() << "配置存放在注册表中，位置在:" << m_config->fileName();
    } else if (m_storageType == TomlFile) {
        // 使用TOML文件存储
        m_tomlFilePath = QDir::currentPath() + "/config.toml";
        // QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        // QDir().mkpath(appDataPath); // 确保目录存在
        // m_tomlFilePath = appDataPath + "/config.toml";
        loadTomlFile();
        qDebug() << "配置存放在TOML文件中，位置在:" << m_tomlFilePath;
    }
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

    if (m_storageType == TomlFile) {
        // TOML文件存储
        QStringList keyParts = key.split('/');
        toml::value *current = &m_tomlData;

        // 导航到正确的嵌套位置
        for (int i = 0; i < keyParts.size() - 1; ++i) {
            const QString &part = keyParts[i];
            if (!current->contains(part.toStdString())) {
                (*current)[part.toStdString()] = toml::table{};
            }
            current = &(*current)[part.toStdString()];
        }

        // 设置值
        const QString &finalKey = keyParts.last();
        (*current)[finalKey.toStdString()] = qVariantToTomlValue(value);

        return saveTomlFile();
    } else {
        // QSettings存储
        if (!m_config)
            return false;
        // if (key == "setting/isDarkMode") {
        //     qDebug() << "当前存储的设置值为" << m_config->value(key, value);
        //     qDebug() << "保存设置" << key << "，值为" << value;
        // }
        m_config->setValue(key, value);
        return m_config->status() == QSettings::NoError;
    }
}

/**
 * @brief 从配置文件读取设置
 * @param key 设置键名
 * @param defaultValue 默认值（如果设置不存在）
 * @return 设置值
 */
QVariant Config::get(const QString &key, const QVariant &defaultValue) {
    // if (key == "setting/isDarkMode") {
    //     qDebug() << "调用get方法, key为" << key << "，当前存储的设置值为" << m_config->value(key, defaultValue);
    // }

    if (m_storageType == TomlFile) {
        // TOML文件存储
        QStringList keyParts = key.split('/');
        const toml::value *current = &m_tomlData;

        // 导航到正确的嵌套位置
        for (const QString &part : keyParts) {
            if (!current->contains(part.toStdString())) {
                return defaultValue;
            }
            current = &current->at(part.toStdString());
        }

        return tomlValueToQVariant(*current);
    } else {
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
}

/**
 * @brief 移除设置
 * @param key 设置键名
 */
void Config::remove(const QString &key) {
    if (m_storageType == TomlFile) {
        // TOML文件存储
        QStringList keyParts = key.split('/');
        toml::value *current = &m_tomlData;

        // 导航到父级位置
        for (int i = 0; i < keyParts.size() - 1; ++i) {
            const QString &part = keyParts[i];
            if (!current->contains(part.toStdString())) {
                return; // 键不存在
            }
            current = &(*current)[part.toStdString()];
        }

        // 移除最后一级键
        const QString &finalKey = keyParts.last();
        if (current->is_table()) {
            auto &table = current->as_table();
            table.erase(finalKey.toStdString());
            saveTomlFile();
        }
    } else {
        if (!m_config)
            return;
        m_config->remove(key);
    }
}

/**
 * @brief 检查设置是否存在
 * @param key 设置键名
 * @return 设置是否存在
 */
bool Config::contains(const QString &key) {
    if (m_storageType == TomlFile) {
        // TOML文件存储
        QStringList keyParts = key.split('/');
        const toml::value *current = &m_tomlData;

        // 导航到正确的嵌套位置
        for (const QString &part : keyParts) {
            if (!current->contains(part.toStdString())) {
                return false;
            }
            current = &current->at(part.toStdString());
        }

        return true;
    } else {
        if (!m_config)
            return false;
        return m_config->contains(key);
    }
}

/**
 * @brief 获取所有设置的键名
 * @return 所有设置的键名列表
 */
QStringList Config::allKeys() {
    if (m_storageType == TomlFile) {
        // TOML文件存储
        QStringList keys;
        collectTomlKeys(m_tomlData, "", keys);
        return keys;
    } else {
        if (!m_config)
            return QStringList();
        return m_config->allKeys();
    }
}

/**
 * @brief 清除所有设置
 */
void Config::clearSettings() {
    if (m_storageType == TomlFile) {
        // TOML文件存储
        m_tomlData = toml::table{};
        saveTomlFile();
    } else {
        if (!m_config)
            return;
        m_config->clear();
    }
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
Config::StorageType Config::getStorageType() const {
    return m_storageType;
}

/**
 * @brief 加载TOML文件
 * @return 加载是否成功
 */
bool Config::loadTomlFile() {
    try {
        if (QFile::exists(m_tomlFilePath)) {
            m_tomlData = toml::parse(m_tomlFilePath.toStdString());
        } else {
            // 如果文件不存在，创建空的TOML数据
            m_tomlData = toml::table{};
        }
        return true;
    } catch (const std::exception &e) {
        qDebug() << "加载TOML文件失败:" << e.what();
        m_tomlData = toml::table{};
        return false;
    }
}

/**
 * @brief 保存TOML文件
 * @return 保存是否成功
 */
bool Config::saveTomlFile() {
    try {
        QFile file(m_tomlFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "无法打开TOML文件进行写入:" << m_tomlFilePath;
            return false;
        }

        std::ostringstream oss;
        oss << m_tomlData;
        file.write(oss.str().c_str());
        file.close();
        return true;
    } catch (const std::exception &e) {
        qDebug() << "保存TOML文件失败:" << e.what();
        return false;
    }
}

/**
 * @brief TOML值转QVariant
 * @param value TOML值
 * @return QVariant值
 */
QVariant Config::tomlValueToQVariant(const toml::value &value) {
    if (value.is_boolean()) {
        return QVariant(value.as_boolean());
    } else if (value.is_integer()) {
        return QVariant(static_cast<int>(value.as_integer()));
    } else if (value.is_floating()) {
        return QVariant(value.as_floating());
    } else if (value.is_string()) {
        return QVariant(QString::fromStdString(value.as_string()));
    } else if (value.is_array()) {
        QVariantList list;
        const auto &arr = value.as_array();
        for (const auto &item : arr) {
            list.append(tomlValueToQVariant(item));
        }
        return QVariant(list);
    }
    return QVariant();
}

/**
 * @brief QVariant转TOML值
 * @param value QVariant值
 * @return TOML值
 */
toml::value Config::qVariantToTomlValue(const QVariant &value) {
    switch (value.typeId()) {
    case QMetaType::Bool:
        return toml::value(value.toBool());
    case QMetaType::Int:
        return toml::value(static_cast<std::int64_t>(value.toInt()));
    case QMetaType::Double:
        return toml::value(value.toDouble());
    case QMetaType::QString:
        return toml::value(value.toString().toStdString());
    case QMetaType::QStringList: {
        toml::array arr;
        const QStringList &list = value.toStringList();
        for (const QString &str : list) {
            arr.push_back(toml::value(str.toStdString()));
        }
        return toml::value(arr);
    }
    case QMetaType::QVariantList: {
        toml::array arr;
        const QVariantList &list = value.toList();
        for (const QVariant &item : list) {
            arr.push_back(qVariantToTomlValue(item));
        }
        return toml::value(arr);
    }
    default:
        return toml::value(value.toString().toStdString());
    }
}

/**
 * @brief 递归收集TOML键
 * @param value TOML值
 * @param prefix 键前缀
 * @param keys 键列表
 */
void Config::collectTomlKeys(const toml::value &value, const QString &prefix, QStringList &keys) {
    if (value.is_table()) {
        const auto &table = value.as_table();
        for (const auto &pair : table) {
            QString key = QString::fromStdString(pair.first);
            QString fullKey = prefix.isEmpty() ? key : prefix + "/" + key;

            if (pair.second.is_table()) {
                collectTomlKeys(pair.second, fullKey, keys);
            } else {
                keys.append(fullKey);
            }
        }
    }
}
