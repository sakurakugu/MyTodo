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
#include <windows.h>
#endif

#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QTextStream>
#include <QTimeZone>
#include <QUrl>
#include <filesystem>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

Config::Config(QObject *parent)
    : QObject(parent),                   //
      m_config(toml::table{}),           //
      m_filePath(getDefaultConfigPath()) //
{
    // 确保配置目录存在
    std::filesystem::path filePath(m_filePath);
    std::filesystem::path dir = filePath.parent_path();
    // 如果目录不存在，创建它
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    // 尝试加载现有配置文件
    loadFromFile();
}

Config::~Config() {
    // 确保所有设置都已保存
    if (m_needsSave) {
        saveToFile();
    }
}

/**
 * @brief 从TOML文件加载配置
 * @return 操作结果或错误信息
 */
void Config::loadFromFile() {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        if (!std::filesystem::exists(m_filePath)) {
            qDebug() << "配置文件不存在，将创建新的配置文件:" << m_filePath;
            return;
        }

        m_config = toml::parse_file(m_filePath);
        qInfo() << "成功加载配置文件:" << m_filePath;

    } catch (const toml::parse_error &e) {
        qCritical() << "解析配置文件失败:" << e.description();
    } catch (const std::ios_base::failure &e) {
        qCritical() << "文件读取失败:" << m_filePath << "错误:" << e.what();
    } catch (const std::exception &e) {
        qCritical() << "加载配置文件时发生未知错误:" << e.what();
    }
}

/**
 * @brief 保存配置到TOML文件
 * @return 操作结果或错误信息
 */
bool Config::saveToFile() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        std::ofstream file(m_filePath);
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

        m_needsSave = false;
        qDebug() << "成功保存配置文件:" << m_filePath;
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
void Config::save(const QString &key, const QVariant &value, std::string_view comment) {
    if (key.isEmpty()) {
        qWarning() << "配置项键名不能为空";
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        // 将键分割为路径
        QStringList parts = key.split('/', Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            qWarning() << "无效的配置项键名:" << key;
            return;
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
        m_needsSave = true; // 标记需要保存
    } catch (const std::exception &e) {
        qCritical() << "保存配置项失败:" << key << "错误:" << e.what();
    } catch (...) {
        qCritical() << "保存配置项失败:" << key << "未知错误";
    }
}

/**
 * @brief 设置配置项
 * @param key 配置项键名
 * @param value 配置项值
 * @return 操作结果或错误信息
 */
void Config::set(const QString &key, const QVariant &value, std::string_view comment) {
    save(key, value, comment);
}

/**
 * @brief 从配置文件读取设置
 * @param key 设置键名
 * @param defaultValue 默认值（如果设置不存在）
 * @return 设置值或错误信息
 */
std::optional<QVariant> Config::get(const QString &key) const {
    if (key.isEmpty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        QStringList parts = key.split('/', Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            return std::nullopt;
        }

        const toml::node *node = &m_config;

        for (const auto &part : parts) {
            if (!node->is_table())
                return std::nullopt;
            node = node->as_table()->get(part.toStdString());
            if (!node)
                return std::nullopt;
        }

        return tomlToVariant(node);
    } catch (const std::exception &e) {
        qWarning() << "获取配置项失败:" << key << "错误:" << e.what();
        return std::nullopt;
    }
}

QVariant Config::get(const QString &key, const QVariant &defaultValue) const {
    auto result = get(key);
    return result.value_or(defaultValue);
}

/**
 * @brief 移除设置
 * @param key 设置键名
 * @return 操作结果或错误信息
 */
void Config::remove(const QString &key) {
    if (key.isEmpty()) {
        qWarning() << "配置项键名不能为空";
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        QStringList parts = key.split('/', Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            qWarning() << "无效的配置项键名:" << key;
            return;
        }

        toml::table *table = &m_config;

        for (int i = 0; i < parts.size() - 1; ++i) {
            auto sub = table->get(parts[i].toStdString());
            if (!sub || !sub->is_table()) {
                qDebug() << "配置项不存在:" << key;
                return;
            }
            table = sub->as_table();
        }

        if (table->erase(parts.last().toStdString())) {
            m_needsSave = true;
            qDebug() << "成功删除配置项:" << key;
        } else {
            qDebug() << "配置项不存在:" << key;
        }

    } catch (const std::exception &e) {
        qCritical() << "删除配置项失败:" << key << "错误:" << e.what();
    }
}

/**
 * @brief 检查设置是否存在
 * @param key 设置键名
 * @return 设置是否存在
 */
bool Config::contains(const QString &key) const noexcept {
    if (key.isEmpty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        QStringList parts = key.split('/', Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            return false;
        }

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
    std::lock_guard<std::mutex> lock(m_mutex);

    QStringList result;
    try {
        collectKeys(m_config, QString(), result);
        return result;
    } catch (const std::exception &e) {
        qWarning() << "获取所有配置项键名失败:" << e.what();
        return QStringList{};
    }
}

/**
 * @brief 清除所有设置
 * @return 操作结果或错误信息
 */
void Config::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        m_config = toml::table{};
        m_needsSave = true;
        qDebug() << "成功清除所有配置项";
    } catch (const std::exception &e) {
        qCritical() << "清除所有设置失败:" << e.what();
    }
}

/**
 * @brief 获取配置文件路径
 * @return 配置文件路径
 */
QString Config::getConfigFilePath() const {
    return QString::fromStdString(m_filePath);
}

/**
 * @brief 打开配置文件路径
 * @return 操作结果或错误信息
 */
bool Config::openConfigFilePath() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        if (m_filePath.empty()) {
            qCritical() << "配置文件路径为空";
            return false;
        }

        QFileInfo fileInfo(QString::fromStdString(m_filePath));
        QString dirPath = fileInfo.absolutePath();

        if (QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath))) {
            qDebug() << "成功打开配置文件目录:" << dirPath;
            return true;
        }

        qCritical() << "打开配置文件路径失败:" << dirPath;
        return false;

    } catch (const std::exception &e) {
        qCritical() << "打开配置文件路径时发生错误:" << e.what();
        return false;
    }
}

/**
 * @brief 获取默认配置文件路径
 * @return 默认配置文件路径
 */
std::string Config::getDefaultConfigPath() const {
    try {
        // 使用当前配置位置设置
        return getConfigLocationPath(m_configLocation);

    } catch (const std::exception &e) {
        qCritical() << "获取默认配置路径失败:" << e.what();
        // 应用程序目录
        return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("config.toml").toStdString();
    }
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
            // 如果是空值或无效时间，保存为最小值 0001-01-01T00:00:00
            if (!dt.isValid() || dt.isNull()) {
                return std::make_unique<toml::value<toml::date_time>>(toml::date_time{{1, 1, 1}, {0, 0, 0, 0}});
            }
            return std::make_unique<toml::value<toml::date_time>>(toml::date_time{
                {dt.date().year(), dt.date().month(), dt.date().day()},
                {dt.time().hour(), dt.time().minute(), dt.time().second(), dt.time().msec() * 1000000}});
        }
        case QMetaType::QDate: {
            auto d = value.toDate();
            // 如果是空值或无效日期，保存为最小值 0001-01-01
            if (!d.isValid() || d.isNull()) {
                return std::make_unique<toml::value<toml::date>>(toml::date{1, 1, 1});
            }
            return std::make_unique<toml::value<toml::date>>(toml::date{d.year(), d.month(), d.day()});
        }
        case QMetaType::QTime: {
            auto t = value.toTime();
            return std::make_unique<toml::value<toml::time>>(
                toml::time{t.hour(), t.minute(), t.second(), t.msec() * 1000000});
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
            QDateTime result(date, time, QTimeZone::UTC);
            // 如果读取到的是最小值 0001-01-01T00:00:00，返回空值
            if (dt.date.year == 1 && dt.date.month == 1 && dt.date.day == 1 && dt.time.hour == 0 &&
                dt.time.minute == 0 && dt.time.second == 0 && dt.time.nanosecond == 0) {
                return QDateTime();
            }
            return result;
        }
        if (auto v = node->as_date()) {
            auto d = v->get();
            // 如果读取到的是最小值 0001-01-01，返回空值
            if (d.year == 1 && d.month == 1 && d.day == 1) {
                return QDate();
            }
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
        QString key = prefix.isEmpty() ? QString::fromStdString(std::string(k))
                                       : prefix + "/" + QString::fromStdString(std::string(k)); // TODO: 这里是.还是/

        if (v.is_table()) {
            collectKeys(*v.as_table(), key, out);
        } else {
            out.append(key);
        }
    }
}

/**
 * @brief 批量设置配置项
 * @param values 配置项映射
 */
void Config::setBatch(const QVariantMap &values) {
    if (values.isEmpty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            if (it.key().isEmpty()) {
                qWarning() << "跳过空键名的配置项";
                continue;
            }

            QStringList parts = it.key().split('/', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "跳过无效键名:" << it.key();
                continue;
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

            auto tomlValue = variantToToml(it.value());
            if (tomlValue) {
                table->insert(parts.last().toStdString(), *tomlValue);
            } else {
                qWarning() << "无法转换配置项值:" << it.key();
            }
        }

        m_needsSave = true;
        qDebug() << "批量设置" << values.size() << "个配置项";

    } catch (const std::bad_alloc &e) {
        qCritical() << "内存不足，批量设置失败:" << e.what();
    } catch (const std::exception &e) {
        qCritical() << "批量设置配置项失败:" << e.what();
    }
}

/**
 * @brief 批量保存配置项
 * @param values 配置项映射
 */
void Config::saveBatch(const QVariantMap &values) {
    setBatch(values);

    // 立即保存到文件
    if (m_needsSave) {
        saveToFile();
    }
}

/**
 * @brief 导出配置为JSON格式
 * @param excludeKeys 要排除的键列表
 * @return JSON字符串
 */
std::string Config::exportToJson(const QStringList &excludeKeys) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        if (excludeKeys.isEmpty()) {
            // 如果没有排除键，直接转换
            std::ostringstream oss;
            oss << toml::json_formatter{m_config};
            return oss.str();
        } else {
            // 如果有排除键，需要创建过滤后的副本
            toml::table filteredConfig;

            // 递归复制配置，排除指定键
            std::function<void(const toml::table &, toml::table &, const QString &)> copyTable =
                [&](const toml::table &source, toml::table &dest, const QString &prefix) {
                    for (auto &[key, value] : source) {
                        QString fullKey = prefix.isEmpty() ? QString::fromStdString(std::string(key))
                                                           : prefix + "/" + QString::fromStdString(std::string(key));

                        // 检查是否在排除列表中
                        if (excludeKeys.contains(fullKey)) {
                            continue;
                        }

                        std::string keyStr = std::string(key);

                        if (value.is_table()) {
                            toml::table nestedTable;
                            copyTable(*value.as_table(), nestedTable, fullKey);
                            if (!nestedTable.empty()) {
                                dest.insert(keyStr, std::move(nestedTable));
                            }
                        } else {
                            // 直接复制值
                            dest.insert(keyStr, value);
                        }
                    }
                };

            copyTable(m_config, filteredConfig, "");

            std::ostringstream oss;
            oss << toml::json_formatter{filteredConfig};
            return oss.str();
        }

    } catch (const std::exception &e) {
        qCritical() << "导出JSON失败:" << e.what();
        return "{}";
    }
}

/**
 * @brief 导出配置到JSON文件
 * @param filePath 文件路径
 * @param excludeKeys 要排除的键列表
 * @return 操作是否成功
 */
bool Config::exportToJsonFile(const QString &filePath, const QStringList &excludeKeys) const {
    try {
        std::string jsonContent = exportToJson(excludeKeys);

        std::ofstream file(filePath.toStdString());
        if (!file.is_open()) {
            qCritical() << "无法打开文件进行写入:" << filePath;
            return false;
        }

        file << jsonContent;
        file.close();

        if (file.fail()) {
            qCritical() << "写入JSON文件失败:" << filePath;
            return false;
        }

        qInfo() << "成功导出配置到JSON文件:" << filePath;
        return true;

    } catch (const std::exception &e) {
        qCritical() << "导出JSON文件失败:" << e.what();
        return false;
    }
}

/**
 * @brief 设置配置文件存放位置
 * @param location 配置文件位置
 */
void Config::setConfigLocation(ConfigLocation location) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_configLocation == location) {
        return; // 位置未改变
    }

    try {
        // 保存当前配置到文件
        if (m_needsSave) {
            saveToFile();
        }

        // 更新位置和文件路径
        m_configLocation = location;
        std::string newPath = getConfigLocationPath(location);

        // 确保新目录存在
        std::filesystem::path newFilePath(newPath);
        std::filesystem::path dir = newFilePath.parent_path();
        if (!std::filesystem::exists(dir)) {
            if (!std::filesystem::create_directories(dir)) {
                throw std::runtime_error("无法创建配置目录: " + dir.string());
            }
        }

        m_filePath = newPath;

        // 重新加载配置
        loadFromFile();

        qDebug() << "配置文件位置已切换到:" << m_filePath;

    } catch (const std::exception &e) {
        qCritical() << "切换配置文件位置失败:" << e.what();
        // 恢复原位置
        m_configLocation = (location == ConfigLocation::ApplicationPath) ? ConfigLocation::AppDataLocal
                                                                         : ConfigLocation::ApplicationPath;
    }
}

/**
 * @brief 获取当前配置文件位置
 * @return 当前配置位置
 */
Config::ConfigLocation Config::getConfigLocation() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_configLocation;
}

/**
 * @brief 获取指定位置的配置文件路径
 * @param location 配置位置
 * @return 配置文件完整路径
 */
std::string Config::getConfigLocationPath(ConfigLocation location) const {
    QString basePath;

    switch (location) {
    case ConfigLocation::ApplicationPath: {
        // 应用程序所在目录
        basePath = QCoreApplication::applicationDirPath();
        break;
    }
    case ConfigLocation::AppDataLocal: {
        // AppData/Local目录
        basePath =
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (basePath.isEmpty()) {
            // 备用方案：使用应用程序目录
            basePath = QCoreApplication::applicationDirPath();
            qWarning() << "无法获取AppData路径，使用应用程序目录作为备用";
        }
        break;
    }
    default:
        basePath = QCoreApplication::applicationDirPath();
        break;
    }

    std::filesystem::path configPath = std::filesystem::path(basePath.toStdString()) / "config.toml";
    return configPath.string();
}
