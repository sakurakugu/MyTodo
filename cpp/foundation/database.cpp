/**
 * @file database.cpp
 * @brief Database 类实现文件
 *
 * 实现 SQLite 数据库连接生命周期管理、版本迁移、线程安全封装、统一数据导入/导出接口注册与调度等功能。
 *
 * @author Sakurakugu
 * @date 2025-09-15 20:55:22 (UTC+8) 周一
 * @change 2025-10-05 21:25:00 (UTC+8) 周日
 */

#include "database.h"
#include "version.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonObject>
#include <QSaveFile>
#include <QSqlError>

Database::Database() : m_initialized(false) {
    // 初始化数据库
    if (!initializeDatabase()) {
        qCritical() << "数据库初始化失败";
    }
}

Database::~Database() {
    // 注意：QSqlDatabase 依赖 QCoreApplication；在应用销毁后调用将导致警告
    if (QCoreApplication::instance()) {
        closeDatabase();
    }
}

/**
 * @brief 初始化数据库连接和表结构
 * @return 初始化是否成功
 */
bool Database::initializeDatabase() {
    std::lock_guard<std::mutex> locker(m_mutex);

    if (m_initialized && m_database.isOpen()) {
        return true;
    }

    // 确保数据库目录存在
    QFileInfo fileInfo(m_databasePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            m_lastError = QString("无法创建数据库目录: %1").arg(dir.absolutePath());
            qCritical() << m_lastError;
            return false;
        }
    }

    // 创建数据库连接
    m_database = QSqlDatabase::addDatabase("QSQLITE", CONNECTION_NAME);
    m_database.setDatabaseName(m_databasePath);

    if (!m_database.open()) {
        m_lastError = QString("无法打开数据库: %1").arg(m_database.lastError().text());
        qCritical() << m_lastError;
        return false;
    }

    // 启用外键约束
    QSqlQuery query(m_database);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        qWarning() << "无法启用外键约束:" << query.lastError().text();
    }

    // 创建版本信息表
    if (!createVersionTable()) {
        m_lastError = "创建版本信息表失败";
        qCritical() << m_lastError;
        return false;
    }

    // 设置数据库版本
    if (!updateDatabaseVersion(DATABASE_VERSION)) {
        m_lastError = "设置数据库版本失败";
        qCritical() << m_lastError;
        return false;
    }

    m_initialized = true;
    qInfo() << "数据库初始化成功:" << m_databasePath << "版本:" << DATABASE_VERSION;
    return true;
}

/**
 * @brief 获取数据库连接
 * @return 数据库连接对象
 */
QSqlDatabase Database::getDatabase() {
    std::lock_guard<std::mutex> locker(m_mutex);

    if (!m_initialized || !m_database.isOpen()) {
        if (!initializeDatabase()) {
            qCritical() << "获取数据库连接失败";
        }
    }

    return QSqlDatabase::database(CONNECTION_NAME);
}

/**
 * @brief 获取数据库连接并检查状态
 * @param db 数据库连接对象的引用
 * @return 数据库连接是否可用
 */
bool Database::getDatabase(QSqlDatabase &db) {
    db = getDatabase();
    return db.isOpen();
}

/**
 * @brief 检查数据库是否已打开
 * @return 数据库是否已打开
 */
bool Database::isDatabaseOpen() const {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_initialized && m_database.isOpen();
}

/**
 * @brief 关闭数据库连接
 */
void Database::closeDatabase() {
    std::lock_guard<std::mutex> locker(m_mutex);

    // 在没有 QCoreApplication 的情况下访问 QSqlDatabase 会触发警告
    if (!QCoreApplication::instance()) {
        return;
    }

    if (m_database.isOpen()) {
        // 等待所有查询完成
        QSqlQuery query(m_database);
        query.clear(); // 清理任何活跃的查询

        m_database.close();
        qInfo() << "数据库连接已关闭";
    }

    // 确保移除数据库连接前，连接确实已关闭
    if (QSqlDatabase::contains(CONNECTION_NAME)) {
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
    }

    m_initialized = false;
}

/**
 * @brief 执行SQL查询
 * @param queryString SQL查询语句
 * @param query 查询对象引用
 * @return 执行是否成功
 */
bool Database::executeQuery(const QString &queryString, QSqlQuery &query) {
    std::lock_guard<std::mutex> locker(m_mutex);

    if (!isDatabaseOpen()) {
        m_lastError = "数据库未打开";
        return false;
    }

    query = QSqlQuery(getDatabase());
    if (!query.exec(queryString)) {
        m_lastError = QString("SQL执行失败: %1 - %2").arg(queryString, query.lastError().text());
        qCritical() << m_lastError;
        return false;
    }

    return true;
}

/**
 * @brief 执行SQL查询（无返回结果）
 * @param queryString SQL查询语句
 * @return 执行是否成功
 */
bool Database::executeQuery(const QString &queryString) {
    QSqlQuery query;
    return executeQuery(queryString, query);
}

/**
 * @brief 获取最后一次错误信息
 * @return 错误信息
 */
QString Database::getLastError() const {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_lastError;
}

/**
 * @brief 获取数据库文件路径
 * @return 数据库文件路径
 */
QString Database::getDatabasePath() const {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_databasePath;
}

/**
 * @brief 获取数据库版本
 * @return 数据库版本号
 */
int Database::getDatabaseVersion() const {
    std::lock_guard<std::mutex> locker(m_mutex);

    if (!isDatabaseOpen()) {
        return 0;
    }

    QSqlQuery query(m_database);
    if (query.exec("SELECT version FROM database_version LIMIT 1") && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

/**
 * @brief 查询数据库版本
 * @return 数据库版本号
 */
QString Database::getSqliteVersion() {
    QSqlQuery query(m_database);
    query.prepare("SELECT sqlite_version();");

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }

    return "未知";
}

/**
 * @brief 导出数据库到JSON文件
 */
bool Database::exportDatabaseToJsonFile(const QString &filePath) {
    QJsonObject root;
    if (!exportDataToJson(root)) {
        return false;
    }

    QJsonDocument doc(root);
    QSaveFile out(filePath);
    if (!out.open(QIODevice::WriteOnly)) {
        m_lastError = QString("无法写入JSON文件: %1").arg(out.errorString());
        qCritical() << m_lastError;
        return false;
    }
    out.write(doc.toJson(QJsonDocument::Indented));
    if (!out.commit()) {
        m_lastError = QString("保存JSON文件失败: %1").arg(out.errorString());
        qCritical() << m_lastError;
        return false;
    }

    qInfo() << "数据库导出成功:" << filePath;
    return true;
}

/**
 * @brief 注册数据导出器
 */
void Database::registerDataExporter(const QString &name, IDataExporter *exporter) {
    std::lock_guard<std::mutex> locker(m_mutex);
    if (exporter) {
        m_dataExporters[name] = exporter;
        // qDebug() << "注册数据导出器:" << name;
    }
}

/**
 * @brief 注销数据导出器
 */
void Database::unregisterDataExporter(const QString &name) {
    std::lock_guard<std::mutex> locker(m_mutex);
    if (m_dataExporters.erase(name)) {
        // qInfo() << "注销数据导出器:" << name;
    }
}

/**
 * @brief 导出所有数据到JSON对象
 */
bool Database::exportDataToJson(QJsonObject &output) {
    std::lock_guard<std::mutex> locker(m_mutex);

    if (!isDatabaseOpen()) {
        m_lastError = "数据库未打开";
        return false;
    }

    // 添加元数据
    output["meta"] = QJsonObject{{"version", QString(APP_VERSION_STRING)},
                                 {"database_version", DATABASE_VERSION},
                                 {"sqlite_version", getSqliteVersion()},
                                 {"export_time", QDateTime::currentDateTime().toString(Qt::ISODate)}};

    // 导出database_version表
    output["database_version"] = exportTable("database_version", {"version"});

    // 通过各个导出器导出数据
    bool allSuccessful = true;
    for (auto it = m_dataExporters.begin(); it != m_dataExporters.end(); ++it) {
        try {
            if (!it->second->导出到JSON(output)) {
                qWarning() << "导出器" << it->first << "导出失败";
                allSuccessful = false;
            }
        } catch (const std::exception &e) {
            qCritical() << "导出器" << it->first << "抛出异常:" << e.what();
            allSuccessful = false;
        }
    }

    return allSuccessful;
}

/**
 * @brief 从JSON对象导入数据
 */
bool Database::importDataFromJson(const QJsonObject &input, bool replaceAll) {
    std::lock_guard<std::mutex> locker(m_mutex);

    if (!isDatabaseOpen()) {
        m_lastError = "数据库未打开";
        return false;
    }

    if (!m_database.transaction()) {
        m_lastError = "开启事务失败";
        return false;
    }

    auto rollback = [&]() {
        if (!m_database.rollback()) {
            qWarning() << "事务回滚失败";
        }
    };

    // 通过各个导出器导入数据
    bool allSuccessful = true;
    for (auto it = m_dataExporters.begin(); it != m_dataExporters.end(); ++it) {
        try {
            if (!it->second->导入从JSON(input, replaceAll)) {
                qWarning() << "导出器" << it->first << "导入失败";
                allSuccessful = false;
                break;
            }
        } catch (const std::exception &e) {
            qCritical() << "导出器" << it->first << "抛出异常:" << e.what();
            allSuccessful = false;
            break;
        }
    }

    if (!allSuccessful) {
        rollback();
        return false;
    }

    // 导入 database_version（如果存在）
    if (input.contains("database_version") && input["database_version"].isArray()) {
        QJsonArray verArr = input["database_version"].toArray();
        if (!verArr.isEmpty()) {
            int ver = verArr.first().toObject().value("version").toInt(DATABASE_VERSION);
            if (!updateDatabaseVersion(ver)) {
                rollback();
                return false;
            }
        }
    }

    if (!m_database.commit()) {
        m_lastError = "提交事务失败";
        qCritical() << m_lastError;
        return false;
    }

    return true;
}

/**
 * @brief 从JSON文件导入数据库（已弃用）
 */
bool Database::importDatabaseFromJsonFile(const QString &filePath, bool replaceAll) {
    QFile in(filePath);
    if (!in.open(QIODevice::ReadOnly)) {
        m_lastError = QString("无法读取JSON文件: %1").arg(in.errorString());
        qCritical() << m_lastError;
        return false;
    }
    QByteArray data = in.readAll();
    in.close();

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        m_lastError = QString("JSON解析失败: %1").arg(err.errorString());
        qCritical() << m_lastError;
        return false;
    }

    return importDataFromJson(doc.object(), replaceAll);
}

/**
 * @brief 创建版本信息表
 * @return 创建是否成功
 */
bool Database::createVersionTable() {
    const QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS database_version (
            version INTEGER PRIMARY KEY
        )
    )";

    QSqlQuery query(m_database);
    if (!query.exec(createTableQuery)) {
        m_lastError = QString("创建database_version表失败: %1").arg(query.lastError().text());
        qCritical() << m_lastError;
        return false;
    }

    return true;
}

/**
 * @brief 数据库版本迁移
 * @param fromVersion 源版本
 * @param toVersion 目标版本
 * @return 迁移是否成功
 */
bool Database::migrateDatabase(int fromVersion, int toVersion) {
    qInfo() << QString("开始数据库迁移: %1 -> %2").arg(fromVersion).arg(toVersion);

    // TODO:
    // 目前只有版本1，暂时不需要迁移逻辑
    // 未来版本升级时再在这里添加迁移代码

    return updateDatabaseVersion(toVersion);
}

/**
 * @brief 更新数据库版本
 * @param version 新版本号
 * @return 更新是否成功
 */
bool Database::updateDatabaseVersion(int version) {
    QSqlQuery query(m_database);

    // 先删除旧版本记录
    if (!query.exec("DELETE FROM database_version")) {
        qWarning() << "删除旧版本记录失败:" << query.lastError().text();
    }

    // 插入新版本记录
    query.prepare("INSERT INTO database_version (version) VALUES (?)");
    query.addBindValue(version);

    if (!query.exec()) {
        m_lastError = QString("更新数据库版本失败: %1").arg(query.lastError().text());
        qCritical() << m_lastError;
        return false;
    }

    return true;
}

/**
 * @brief 检查表是否存在
 * @param tableName 表名
 * @return 表是否存在
 */
bool Database::tableExists(const QString &tableName) {
    QSqlQuery query(m_database);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=?");
    query.addBindValue(tableName);

    if (query.exec() && query.next()) {
        return true;
    }

    return false;
}

/**
 * @brief 导出指定表到JSON数组
 */
QJsonArray Database::exportTable(const QString &table, const QStringList &columns) {
    QJsonArray array;
    QSqlQuery q(m_database);
    QString sql = QString("SELECT %1 FROM %2").arg(columns.join(", ")).arg(table);
    if (!q.exec(sql)) {
        qWarning() << "导出表失败:" << table << q.lastError().text();
        return array;
    }
    while (q.next()) {
        QJsonObject obj;
        for (int i = 0; i < columns.size(); ++i) {
            auto v = q.value(i);
            if (v.typeId() == QMetaType::Bool)
                obj[columns[i]] = v.toBool();
            else if (v.typeId() == QMetaType::Int || v.typeId() == QMetaType::LongLong)
                obj[columns[i]] = QJsonValue::fromVariant(v);
            else if (v.typeId() == QMetaType::Double)
                obj[columns[i]] = v.toDouble();
            else if (v.isNull())
                obj[columns[i]] = QJsonValue();
            else
                obj[columns[i]] = v.toString();
        }
        array.append(obj);
    }
    return array;
}
