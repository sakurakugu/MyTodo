/**
 * @file config.cpp
 * @brief 配置类实现
 * @author sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-08-31 15:07:38(UTC+8) 周日
 * @version 0.4.0
 */
#include "config.h"

#if defined(Q_OS_WIN)
#include <QProcess>
#include <windows.h>
#endif

#include <QDesktopServices>
#include <QFileInfo>
#include <QTextStream>
#include <QUrl>
#include <toml11/datetime.hpp>

Config::Config(QObject *parent) : QObject(parent), m_configFilePath(getDefaultConfigPath()) {
    // 确保配置目录存在
    QFileInfo fileInfo(m_configFilePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 尝试加载现有配置文件
    loadFromFile();

    qDebug() << "配置存放在:" << m_configFilePath;
}

Config::~Config() noexcept {
    // 确保所有设置都已保存
    saveToFile();
}

/**
 * @brief 保存设置到配置文件
 * @param key 设置键名
 * @param value 设置值
 * @return 操作结果或错误信息
 */
bool Config::save(QStringView key, const QVariant &value) noexcept {
    try {
        QString keyStr = key.toString();

        // 处理嵌套键（如 "section.subsection.key"）
        QStringList keyParts = keyStr.split('/');
        toml::value *current = &m_tomlData;

        for (int i = 0; i < keyParts.size() - 1; ++i) {
            std::string part = keyParts[i].toStdString();
            if (!current->contains(part)) {
                (*current)[part] = toml::table{};
            }
            current = &(*current)[part];
        }

        std::string finalKey = keyParts.last().toStdString();
        (*current)[finalKey] = variantToToml(value);

        m_isChanged = true;
        return saveToFile() ? true : false;
    } catch (...) {
        qCritical() << "保存配置项失败";
        return false;
    }
}

/**
 * @brief 从配置文件读取设置
 * @param key 设置键名
 * @param defaultValue 默认值（如果设置不存在）
 * @return 设置值或错误信息
 */
QVariant Config::get(QStringView key, const QVariant &defaultValue) const noexcept {
    try {
        QString keyStr = key.toString();
        QStringList keyParts = keyStr.split('/');
        const toml::value *current = &m_tomlData;

        // 遍历嵌套键
        for (const QString &part : keyParts) {
            std::string partStr = part.toStdString();
            if (!current->contains(partStr)) {
                return defaultValue;
            }
            current = &current->at(partStr);
        }

        return tomlToVariant(*current);
    } catch (...) {
        return defaultValue;
    }
}

/**
 * @brief 移除设置
 * @param key 设置键名
 * @return 操作结果或错误信息
 */
void Config::remove(QStringView key) noexcept {
    try {
        QString keyStr = key.toString();
        QStringList keyParts = keyStr.split('/');
        toml::value *current = &m_tomlData;

        // 找到父级容器
        for (int i = 0; i < keyParts.size() - 1; ++i) {
            std::string part = keyParts[i].toStdString();
            if (!current->contains(part)) {
                qCritical() << "删除配置项失败，键不存在:" << keyStr;
                return;
            }
            current = &(*current)[part];
        }

        std::string finalKey = keyParts.last().toStdString();
        if (!current->contains(finalKey)) {
            qCritical() << "删除配置项失败，键不存在:" << keyStr;
            return;
        }

        current->as_table().erase(finalKey);
        m_isChanged = true;
        saveToFile();
    } catch (...) {
        qCritical() << "删除配置项失败";
    }
}

/**
 * @brief 检查设置是否存在
 * @param key 设置键名
 * @return 设置是否存在
 */
bool Config::contains(QStringView key) const noexcept {
    try {
        QString keyStr = key.toString();
        QStringList keyParts = keyStr.split('/');
        const toml::value *current = &m_tomlData;

        for (const QString &part : keyParts) {
            std::string partStr = part.toStdString();
            if (!current->contains(partStr)) {
                return false;
            }
            current = &current->at(partStr);
        }
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * @brief 获取所有设置的键名
 * @return 所有设置的键名列表
 */
QStringList Config::allKeys() const noexcept {
    QStringList keys;
    try {
        std::function<void(const toml::value &, const QString &)> collectKeys = [&](const toml::value &value,
                                                                                    const QString &prefix) {
            if (value.is_table()) {
                for (const auto &[key, val] : value.as_table()) {
                    QString fullKey =
                        prefix.isEmpty() ? QString::fromStdString(key) : prefix + "/" + QString::fromStdString(key);
                    if (val.is_table()) {
                        collectKeys(val, fullKey);
                    } else {
                        keys.append(fullKey);
                    }
                }
            }
        };
        collectKeys(m_tomlData, "");
    } catch (...) {
        // 返回空列表
    }
    return keys;
}

/**
 * @brief 清除所有设置
 * @return 操作结果或错误信息
 */
void Config::clear() noexcept {
    try {
        m_tomlData = toml::table{};
        m_isChanged = true;
        saveToFile();
    } catch (...) {
        qCritical() << "清除所有设置失败";
    }
}

/**
 * @brief 获取配置文件路径
 * @return 配置文件路径
 */
QString Config::getConfigFilePath() const noexcept {
    return m_configFilePath;
}

/**
 * @brief 打开配置文件路径
 * @return 操作结果或错误信息
 */
bool Config::openConfigFilePath() const noexcept {
    try {
        if (m_configFilePath.isEmpty()) {
            qCritical() << "配置文件路径为空";
            return false;
        }

        if (QDesktopServices::openUrl(QUrl::fromLocalFile(m_configFilePath))) {
            return true;
        }
        qCritical() << "打开配置文件路径失败" << m_configFilePath;
        return false;
    } catch (...) {
        qCritical() << "打开配置文件路径失败" << m_configFilePath;
        return false;
    }
}

/**
 * @brief 从TOML文件加载配置
 * @return 操作结果或错误信息
 */
void Config::loadFromFile() const noexcept {
    try {
        if (QFile::exists(m_configFilePath)) {
            m_tomlData = toml::parse(m_configFilePath.toStdString());
        } else {
            // 如果文件不存在，创建空的TOML数据
            m_tomlData = toml::table{};
        }
    } catch (const std::exception &e) {
        qDebug() << "加载TOML文件失败:" << e.what();
        m_tomlData = toml::table{};
    }
}

/**
 * @brief 保存配置到TOML文件
 * @return 操作结果或错误信息
 */
bool Config::saveToFile() const noexcept {
    try {
        std::ofstream file(m_configFilePath.toStdString());
        if (!file.is_open()) {
            qCritical() << "写入配置文件失败，文件无法打开:" << m_configFilePath;
            return false;
        }

        file << m_tomlData;
        file.close();

        if (file.fail()) {
            qCritical() << "写入配置文件失败，文件写入失败:" << m_configFilePath;
            return false;
        }

        m_isChanged = false;
        return true;
    } catch (...) {
        qCritical() << "写入配置文件失败:" << m_configFilePath;
        return false;
    }
}

/**
 * @brief 获取默认配置文件路径
 * @return 默认配置文件路径
 */
QString Config::getDefaultConfigPath() const noexcept {
    QString configDir = QDir::currentPath();
    if (configDir.isEmpty()) {
        configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }
    return configDir + "/config.toml";
}

/**
 * @brief 将QVariant转换为toml::value
 * @param value QVariant值
 * @return toml::value
 */
toml::value Config::variantToToml(const QVariant &value) const noexcept {
    try {
        switch (value.typeId()) {
        case QMetaType::Bool:
            return toml::value(value.toBool());
        case QMetaType::Int:
            return toml::value(value.toInt());
        case QMetaType::LongLong:
            return toml::value(value.toLongLong());
        case QMetaType::Double:
            return toml::value(value.toDouble());
        case QMetaType::QString:
            return toml::value(value.toString().toStdString());
        case QMetaType::QStringList: {
            QStringList list = value.toStringList();
            toml::array arr;
            for (const QString &str : list) {
                arr.push_back(toml::value(str.toStdString()));
            }
            return toml::value(arr);
        }
        case QMetaType::QVariantList: {
            toml::array arr;
            const QVariantList &list = value.toList();
            for (const QVariant &item : list) {
                arr.push_back(variantToToml(item));
            }
            return toml::value(arr);
        }
        default:
            // 对于其他类型，转换为字符串
            return toml::value(value.toString().toStdString());
        }
    } catch (...) {
        qCritical() << "转换为toml::value失败";
    }
    return toml::value();
}
/**
 * @brief 将toml::value转换为QVariant
 * @param value toml::value
 * @return QVariant
 */
QVariant Config::tomlToVariant(const toml::value &value) const noexcept {
    try {
        if (value.is_boolean()) {
            return QVariant(value.as_boolean());
        } else if (value.is_integer()) {
            return QVariant(static_cast<qint64>(value.as_integer()));
        } else if (value.is_floating()) {
            return QVariant(value.as_floating());
        } else if (value.is_string()) {
            return QVariant(QString::fromStdString(value.as_string()));
        } else if (value.is_array()) {
            QStringList list;
            for (const auto &item : value.as_array()) {
                if (item.is_string()) {
                    list.append(QString::fromStdString(item.as_string()));
                } else {
                    // 其他类型，转换为字符串
                    list.append(QString::fromStdString(toml::format(item)));
                }
            }
            return QVariant(list);
        } else {
            // 对于其他类型，转换为字符串
            return QVariant(QString::fromStdString(toml::format(value)));
        }
    } catch (...) {
        return QVariant();
    }
}
