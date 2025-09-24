/**
 * @file config.cpp
 * @brief 配置类实现
 * @author sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-10 16:24:14(UTC+8) 周三
 */
#include "config.h"
#include "version.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUrl>

Config::Config(QObject *parent)
    : QObject(parent),        //
      m_config(toml::table{}) //
{
    findExistingConfigFile();            // 先查找是否有现有配置文件，确定配置位置
    m_filePath = getDefaultConfigPath(); // 设置配置文件路径

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

Config::~Config() {}

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
    if (key.isEmpty()) {
        qWarning() << "配置项键名不能为空";
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        try {
            // 将键分割为路径
            QStringList parts = key.split('/', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "无效的配置项键名:" << key;
                return;
            }

            toml::table *table = &m_config; // 指向根表的指针

            // 获取或创建嵌套表
            for (int i = 0; i < parts.size() - 1; ++i) {
                const std::string k = parts[i].toStdString();
                if (!table->contains(k) || !(*table)[k].is_table()) {
                    (*table).insert(k, toml::table{});
                }
                table = (*table)[k].as_table();
            }

            // 插入或赋值
            table->insert_or_assign(parts.last().toStdString(), *variantToToml(value));

        } catch (const std::exception &e) {
            qCritical() << "保存配置项失败:" << key << "错误:" << e.what();
            return;
        } catch (...) {
            qCritical() << "保存配置项失败:" << key << "未知错误";
            return;
        }
    } // 锁在这里自动释放

    // 立即保存到文件
    saveToFile();
}

/**
 * @brief 从配置文件读取设置
 * @param key 设置键名
 * @param defaultValue 默认值（如果设置不存在）
 * @return 设置值或错误信息
 */
QVariant Config::get(const QString &key) const {
    if (key.isEmpty()) {
        return QVariant();
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        QStringList parts = key.split('/', Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            return QVariant();
        }

        const toml::node *node = &m_config;

        for (const auto &part : parts) {
            if (!node->is_table())
                return QVariant();
            node = node->as_table()->get(part.toStdString());
            if (!node)
                return QVariant();
        }

        if (node->is_array()) {
            return QVariant::fromValue(tomlToVariant(node));
        }

        return tomlToVariant(node);
    } catch (const std::exception &e) {
        qWarning() << "获取配置项失败:" << key << "错误:" << e.what();
        return QVariant();
    }
}

QVariant Config::get(const QString &key, const QVariant &defaultValue) const {
    auto result = get(key);
    return result.isValid() ? result : defaultValue;
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
    bool needSave = false;
    {
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
                needSave = true;
                qDebug() << "成功删除配置项:" << key;
            } else {
                qDebug() << "配置项不存在:" << key;
            }

        } catch (const std::exception &e) {
            qCritical() << "删除配置项失败:" << key << "错误:" << e.what();
        }
    } // 锁在这里自动释放

    // 如果有删除操作，立即保存到文件
    if (needSave) {
        saveToFile();
    }
}

/**
 * @brief 查找配置项
 * @param key 配置项键名
 * @return 配置项节点指针
 */
const toml::node *Config::find(const QString &key) const noexcept {
    if (key.isEmpty()) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        QStringList parts = key.split('/', Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            return nullptr;
        }

        const toml::node *node = &m_config;

        for (const auto &part : parts) {
            if (!node->is_table())
                return nullptr;
            node = node->as_table()->get(part.toStdString());
            if (!node)
                return nullptr;
        }

        return node; // 返回最终节点
    } catch (...) {
        return nullptr;
    }
}

/**
 * @brief 检查设置是否存在
 * @param key 设置键名
 * @return 设置是否存在
 */
bool Config::contains(const QString &key) const noexcept {
    return find(key) != nullptr;
}

/**
 * @brief 清除所有设置
 * @return 操作结果或错误信息
 */
void Config::clear() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        try {
            m_config = toml::table{};
            qDebug() << "成功清除所有配置项";
        } catch (const std::exception &e) {
            qCritical() << "清除所有设置失败:" << e.what();
            return;
        }
    } // 锁在这里自动释放

    // 立即保存到文件
    saveToFile();
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
/**
 * @brief 查找现有配置文件并更新配置位置
 */
void Config::findExistingConfigFile() {
    // 按优先级顺序查找配置文件
    std::vector<ConfigLocation> searchOrder = {ConfigLocation::ApplicationPath, ConfigLocation::AppDataLocal};

    for (const auto &location : searchOrder) {
        std::string configPath = getConfigLocationPath(location);
        if (std::filesystem::exists(configPath)) {
            m_configLocation = location;
            return;
        }
    }

    // 如果没有找到现有配置文件，使用默认位置
    qDebug() << "未找到现有配置文件，使用默认位置";
}

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
            // 如果是空值或无效时间，保存为最小值 1970-01-01T00:00:00
            if (!dt.isValid() || dt.isNull()) {
                return std::make_unique<toml::value<toml::date_time>>(toml::date_time{{1970, 1, 1}, {0, 0, 0, 0}});
            }
            return std::make_unique<toml::value<toml::date_time>>(toml::date_time{
                {dt.date().year(), dt.date().month(), dt.date().day()},
                {dt.time().hour(), dt.time().minute(), dt.time().second(), dt.time().msec() * 1000000}});
        }
        case QMetaType::QDate: {
            auto d = value.toDate();
            // 如果是空值或无效日期，保存为最小值 1970-01-01
            if (!d.isValid() || d.isNull()) {
                return std::make_unique<toml::value<toml::date>>(toml::date{1970, 1, 1});
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
            // 如果读取到的是最小值 1970-01-01T00:00:00，返回空值
            if (dt.date.year == 1970 && dt.date.month == 1 && dt.date.day == 1 && dt.time.hour == 0 &&
                dt.time.minute == 0 && dt.time.second == 0 && dt.time.nanosecond == 0) {
                return QDateTime();
            }
            return result;
        }
        if (auto v = node->as_date()) {
            auto d = v->get();
            // 如果读取到的是最小值 1970-01-01，返回空值
            if (d.year == 1970 && d.month == 1 && d.day == 1) {
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
 * @brief 批量设置配置项
 * @param values 配置项映射
 */
void Config::setBatch(const QVariantMap &values) {
    if (values.isEmpty()) {
        return;
    }

    {
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

            qDebug() << "批量设置" << values.size() << "个配置项";

        } catch (const std::bad_alloc &e) {
            qCritical() << "内存不足，批量设置失败:" << e.what();
            return;
        } catch (const std::exception &e) {
            qCritical() << "批量设置配置项失败:" << e.what();
            return;
        }
    } // 锁在这里自动释放

    // 立即保存到文件
    saveToFile();
}

/**
 * @brief 批量保存配置项
 * @param values 配置项映射
 */
void Config::saveBatch(const QVariantMap &values) {
    // setBatch 已经会自动保存到文件，所以直接调用即可
    setBatch(values);
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

            filteredConfig.insert("export_info/version", APP_VERSION_STRING); // 应用版本
            filteredConfig.insert("export_info/export_time",
                                  QDateTime::currentDateTime().toString(Qt::ISODate).toStdString()); // 添加导出时间

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
 * @brief JSON字符串 转换为 TOML
 * @param jsonContent JSON字符串内容
 * @param table 导出的table表
 * @return 操作是否成功
 */
bool Config::JsonToToml(const std::string &jsonContent, toml::table *table) {

    try {
        if (jsonContent.empty()) {
            qWarning() << "JSON内容为空";
            return false;
        }

        // 解析JSON
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(QByteArray::fromStdString(jsonContent), &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qCritical() << "JSON解析失败:" << parseError.errorString();
            return false;
        }

        if (!jsonDoc.isObject()) {
            qCritical() << "JSON根节点必须是对象";
            return false;
        }

        QJsonObject jsonObj = jsonDoc.object();

        // 递归转换JSON对象为TOML表
        std::function<std::unique_ptr<toml::node>(const QJsonValue &)> jsonToToml =
            [&](const QJsonValue &value) -> std::unique_ptr<toml::node> {
            switch (value.type()) {
            case QJsonValue::Bool:
                return std::make_unique<toml::value<bool>>(value.toBool());
            case QJsonValue::Double: {
                double d = value.toDouble();
                // 检查是否为整数
                if (d == std::floor(d) && d >= std::numeric_limits<int64_t>::min() &&
                    d <= std::numeric_limits<int64_t>::max()) {
                    return std::make_unique<toml::value<int64_t>>(static_cast<int64_t>(d));
                } else {
                    return std::make_unique<toml::value<double>>(d);
                }
            }
            case QJsonValue::String: {
                QString str = value.toString();
                // 尝试解析为日期时间
                QDateTime dt = QDateTime::fromString(str, Qt::ISODate);
                if (dt.isValid()) {
                    dt = dt.toUTC();
                    return std::make_unique<toml::value<toml::date_time>>(toml::date_time{
                        {dt.date().year(), dt.date().month(), dt.date().day()},
                        {dt.time().hour(), dt.time().minute(), dt.time().second(), dt.time().msec() * 1000000}});
                }
                QDate date = QDate::fromString(str, "yyyy-MM-dd");
                if (date.isValid()) {
                    return std::make_unique<toml::value<toml::date>>(toml::date{date.year(), date.month(), date.day()});
                }
                return std::make_unique<toml::value<std::string>>(str.toStdString());
            }
            case QJsonValue::Array: {
                auto arr = std::make_unique<toml::array>();
                QJsonArray jsonArray = value.toArray();
                for (const auto &item : jsonArray) {
                    auto tomlItem = jsonToToml(item);
                    if (tomlItem) {
                        arr->push_back(*tomlItem);
                    }
                }
                return arr;
            }
            case QJsonValue::Object: {
                auto tbl = std::make_unique<toml::table>();
                QJsonObject jsonObject = value.toObject();
                for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it) {
                    auto tomlValue = jsonToToml(it.value());
                    if (tomlValue) {
                        tbl->insert(it.key().toStdString(), *tomlValue);
                    }
                }
                return tbl;
            }
            case QJsonValue::Null:
            case QJsonValue::Undefined:
            default:
                return std::make_unique<toml::value<std::string>>(std::string{});
            }
        };

        // 转换JSON对象为TOML表
        auto newTomlNode = jsonToToml(QJsonValue(jsonObj));
        if (!newTomlNode || !newTomlNode->is_table()) {
            qCritical() << "JSON转换为TOML失败";
            return false;
        }

        *table = *newTomlNode->as_table();
        return true;

    } catch (const std::exception &e) {
        qCritical() << "将JSON字符串转化为Toml格式失败:" << e.what();
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

    ConfigLocation oldLocation = m_configLocation;
    std::string oldPath = m_filePath;

    try {
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

        // 如果旧配置文件存在，复制到新位置
        if (std::filesystem::exists(oldPath) && oldPath != newPath) {
            std::filesystem::copy_file(oldPath, newPath, std::filesystem::copy_options::overwrite_existing);
            qDebug() << "配置文件已复制到新位置:" << newPath;
        }

        m_filePath = newPath;

        // 重新加载配置
        loadFromFile();

        // 确认新配置文件加载成功后，删除旧配置文件
        if (std::filesystem::exists(oldPath) && oldPath != newPath) {
            try {
                std::filesystem::remove(oldPath);
                qDebug() << "已删除旧配置文件:" << oldPath;
            } catch (const std::exception &e) {
                qWarning() << "删除旧配置文件失败:" << oldPath << "错误:" << e.what();
            }
        }

        qDebug() << "配置文件位置已切换到:" << m_filePath;

    } catch (const std::exception &e) {
        qCritical() << "切换配置文件位置失败:" << e.what();
        // 恢复原位置
        m_configLocation = oldLocation;
        m_filePath = oldPath;
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
        basePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
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

    // 使用QDir来确保路径分隔符正确
    QDir baseDir(basePath);
    QString configPath = baseDir.absoluteFilePath("config.toml");

    // 将路径分隔符统一为正斜杠
    configPath = QDir::fromNativeSeparators(configPath);

    return configPath.toStdString();
}
