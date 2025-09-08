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
#include <QTimeZone>
#include <QUrl>
#include <functional>
#include <sstream>

Config::Config(QObject *parent) : QObject(parent), m_filePath(getDefaultConfigPath()) {
    // 确保配置目录存在
    QFileInfo fileInfo(m_filePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 尝试加载现有配置文件
    loadFromFile();

    qDebug() << "配置存放在:" << m_filePath;
}

Config::~Config() {
    // 确保所有设置都已保存
    saveToFile();
}

/**
 * @brief 从TOML文件加载配置
 * @return 操作结果或错误信息
 */
void Config::loadFromFile() {
    try {
        m_config = toml::parse_file(m_filePath.toStdString());

    } catch (const toml::parse_error &e) {
        qCritical() << "解析配置文件失败:" << e.description();
    } catch (const std::exception &e) {
        qCritical() << "意外错误:" << e.what();
    }
}

/**
 * @brief 保存配置到TOML文件
 * @return 操作结果或错误信息
 */
bool Config::saveToFile() const {
    try {
        std::ofstream file(m_filePath.toStdString());
        if (!file.is_open()) {
            qCritical() << "写入配置文件失败，文件无法打开:" << m_filePath;
            return false;
        }

        file << m_config;
        file.close();

        if (file.fail()) {
            qCritical() << "写入配置文件失败，文件写入失败:" << m_filePath;
            return false;
        }

        return true;
    } catch (...) {
        qCritical() << "写入配置文件失败:" << m_filePath;
        return false;
    }
}

/**
 * @brief 保存设置到配置文件
 * @param key 设置键名
 * @param value 设置值
 */
void Config::save(const QString &key, const QVariant &value) {
    try {
        // 将键分割为路径
        QStringList parts = key.split('/');
        if (parts.isEmpty()) {
        }
        toml::table *table = &m_config;

        // 获取或创建嵌套表
        for (int i = 0; i < parts.size() - 1; ++i) {
            const std::string k = parts[i].toStdString();
            if (!table->contains(k) || !(*table)[k].is_table()) {
                (*table).insert(k, toml::table{});
            }
            table = (*table)[k].as_table();
        }

        table->insert(parts.last().toStdString(), *variantToToml(value));
    } catch (...) {
        qCritical() << "保存配置项失败";
    }
}

/**
 * @brief 设置配置项
 * @param key 配置项键名
 * @param value 配置项值
 * @return 操作结果或错误信息
 */
void Config::set(const QString &key, const QVariant &value) {
    save(key, value);
}

/**
 * @brief 从配置文件读取设置
 * @param key 设置键名
 * @param defaultValue 默认值（如果设置不存在）
 * @return 设置值或错误信息
 */
QVariant Config::get(const QString &key, const QVariant &defaultValue) const {
    try {
        QStringList parts = key.split('/');
        const toml::node *node = &m_config;

        for (const auto &part : parts) {
            if (!node->is_table())
                return defaultValue;
            node = node->as_table()->get(part.toStdString());
            if (!node)
                return defaultValue;
        }

        return tomlToVariant(node);
    } catch (...) {
        return defaultValue;
    }
}

/**
 * @brief 移除设置
 * @param key 设置键名
 * @return 操作结果或错误信息
 */
void Config::remove(const QString &key) {
    try {
        QStringList parts = key.split('/');
        toml::table *table = &m_config;

        for (int i = 0; i < parts.size() - 1; ++i) {
            auto sub = table->get(parts[i].toStdString());
            if (!sub || !sub->is_table()) {
                break;
            }
            table = sub->as_table();
        }

        table->erase(parts.last().toStdString());
    } catch (...) {
        qCritical() << "删除配置项失败";
    }
}

/**
 * @brief 检查设置是否存在
 * @param key 设置键名
 * @return 设置是否存在
 */
bool Config::contains(const QString &key) const {
    try {
        QStringList parts = key.split('/');
        const toml::node *node = &m_config;

        for (const auto &part : parts) {
            if (!node->is_table())
                return false;
            node = node->as_table()->get(part.toStdString());
            if (!node)
                return false;
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
QStringList Config::allKeys() const {
    QStringList result;
    try {
        collectKeys(m_config, QString(), result);
        return result;
    } catch (...) {
        // 发生异常时返回空列表
    }
    return result;
}

/**
 * @brief 清除所有设置
 * @return 操作结果或错误信息
 */
void Config::clear() {
    try {
        m_config = toml::table{};
    } catch (...) {
        qCritical() << "清除所有设置失败";
    }
}

/**
 * @brief 获取配置文件路径
 * @return 配置文件路径
 */
QString Config::getConfigFilePath() const {
    return m_filePath;
}

/**
 * @brief 打开配置文件路径
 * @return 操作结果或错误信息
 */
bool Config::openConfigFilePath() const {
    try {
        if (m_filePath.isEmpty()) {
            qCritical() << "配置文件路径为空";
            return false;
        }

        if (QDesktopServices::openUrl(QUrl::fromLocalFile(m_filePath))) {
            return true;
        }
        qCritical() << "打开配置文件路径失败" << m_filePath;
        return false;
    } catch (...) {
        qCritical() << "打开配置文件路径失败" << m_filePath;
        return false;
    }
}

/**
 * @brief 获取默认配置文件路径
 * @return 默认配置文件路径
 */
QString Config::getDefaultConfigPath() const {
    QString configDir = QDir::currentPath();
    if (configDir.isEmpty()) {
        configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }
    return configDir + "/config.toml";
}

/**
 * @brief 将QVariant转换为toml::value
 * @param value QVariant值
 * @return toml::value值
 */
std::unique_ptr<toml::node> Config::variantToToml(const QVariant &value) {
    try {
        switch (value.typeId()) {
        case QMetaType::Bool:
            return std::make_unique<toml::value<bool>>(value.toBool());
        case QMetaType::Int:
             return std::make_unique<toml::value<int64_t>>(value.toInt());
        case QMetaType::LongLong:
            return std::make_unique<toml::value<int64_t>>(value.toLongLong());
        case QMetaType::Double:
            return std::make_unique<toml::value<double>>(value.toDouble());
        case QMetaType::QString:
            return std::make_unique<toml::value<std::string>>(value.toString().toStdString());
        case QMetaType::QDateTime: {
            auto dt = value.toDateTime().toUTC();
            return std::make_unique<toml::value<toml::date_time>>(toml::date_time{
                {dt.date().year(), dt.date().month(), dt.date().day()},
                {dt.time().hour(), dt.time().minute(), dt.time().second(), dt.time().msec() * 1000000}});
        }
        case QMetaType::QDate: {
            auto d = value.toDate();
            return std::make_unique<toml::value<toml::date>>(toml::date{d.year(), d.month(), d.day()});
        }
        case QMetaType::QTime: {
            auto t = value.toTime();
            return std::make_unique<toml::value<toml::time>>(toml::time{t.hour(), t.minute(), t.second(), t.msec() * 1000000});
        }
        case QMetaType::QByteArray: {
            QString hex = value.toByteArray().toHex();
            return std::make_unique<toml::value<std::string>>(hex.toStdString());
        }
        case QMetaType::QStringList: {
            auto arr = std::make_unique<toml::array>();
            for (const auto &s : value.toStringList()) {
                arr->push_back(s.toStdString());
            }
            return arr;
        }
        case QMetaType::QVariantList: {
            auto arr = std::make_unique<toml::array>();
            for (const auto &v : value.toList()) {
                arr->push_back(*variantToToml(v));
            }
            return arr;
        }
        case QMetaType::QVariantMap: {
            auto tbl = std::make_unique<toml::table>();
            for (auto it = value.toMap().cbegin(); it != value.toMap().cend(); ++it) {
                tbl->insert(it.key().toStdString(), *variantToToml(it.value()));
            }
            return tbl;
        }
        default:
            // 对于不支持的类型，转换为字符串
            return std::make_unique<toml::value<std::string>>(value.toString().toStdString());
        }
    } catch (...) {
        // 转换失败时返回空字符串
        return std::make_unique<toml::value<std::string>>(std::string{});
    }
}

/**
 * @brief 将toml::value转换为QVariant
 * @param value toml::value值
 * @return QVariant值
 */
QVariant Config::tomlToVariant(const toml::node *node) const {
    try {
        if (!node)
            return {};
        if (auto v = node->as_boolean()) {
            return QVariant::fromValue<bool>(v->get());
        }
        if (auto v = node->as_integer()) {
            return QVariant::fromValue<int>(static_cast<int>(v->get()));
        }
        if (auto v = node->as_floating_point()) {
            return QVariant::fromValue<double>(v->get());
        }
        if (auto v = node->as_string()) {
            return QVariant(QString::fromStdString(std::string(v->get())));
        }
        if (auto v = node->as_date_time()) {
            auto dt = v->get();
            QDate date(dt.date.year, dt.date.month, dt.date.day);
            QTime time(dt.time.hour, dt.time.minute, dt.time.second, dt.time.nanosecond / 1000000);
            return QDateTime(date, time, QTimeZone::UTC);
        }
        if (auto v = node->as_date()) {
            auto d = v->get();
            return QDate(d.year, d.month, d.day);
        }
        if (auto v = node->as_time()) {
            auto t = v->get();
            return QTime(t.hour, t.minute, t.second, t.nanosecond / 1000000);
        }
        if (auto arr = node->as_array()) {
            QVariantList list;
            for (auto &elem : *arr) {
                list.append(tomlToVariant(&elem));
            }
            return list;
        }
        if (auto tbl = node->as_table()) {
            QVariantMap map;
            for (auto &[k, v] : *tbl) {
                map[QString::fromStdString(std::string(k))] = tomlToVariant(&v);
            }
            return map;
        }
    } catch (...) {
        // 转换失败时返回空QVariant
    }
    return {};
}

/**
 * @brief 递归收集所有键
 * @param tbl 配置表
 * @param prefix 前缀
 * @param out 输出列表
 */
void Config::collectKeys(const toml::table &tbl, const QString &prefix, QStringList &out) const {
    for (auto &[k, v] : tbl) {
        QString key = prefix.isEmpty() ? QString::fromStdString(std::string(k)) : prefix + "." + QString::fromStdString(std::string(k));

        if (v.is_table()) {
            collectKeys(*v.as_table(), key, out);
        } else {
            out.append(key);
        }
    }
}
