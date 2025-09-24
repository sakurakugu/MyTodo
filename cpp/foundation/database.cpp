/**
 * @file database.cpp
 * @brief DatabaseManager类的实现
 *
 * 该文件实现了DatabaseManager类，用于管理应用程序的SQLite数据库连接和初始化。
 *
 * @author Sakurakugu
 * @date 2025-09-15 20:55:22(UTC+8) 周一
 * @change 2025-09-22 19:36:18(UTC+8) 周一
 */

#include "database.h"
#include "config.h"
#include "default_value.h"
#include "version.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>
#include <mutex>

Database::Database(QObject *parent) : QObject(parent), m_initialized(false) {
    m_databasePath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                         .absoluteFilePath(QString(DefaultValues::appName) + ".db");

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

    // 创建表结构
    if (!createTables()) {
        m_lastError = "创建数据库表失败";
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
    std::lock_guard<std::mutex> locker(m_mutex);

    if (!isDatabaseOpen()) {
        m_lastError = "数据库未打开";
        return false;
    }

    auto exportTable = [&](const QString &table, const QStringList &columns) -> QJsonArray {
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
    };

    QJsonObject root;
    root["meta"] = QJsonObject //
        {
            {"version", QString(APP_VERSION_STRING)},
            {"database_version", DATABASE_VERSION},
            {"sqlite", getSqliteVersion()},
            {"export_time", QDateTime::currentDateTime().toString(Qt::ISODate)} //
        };

    // 导出各表
    root["database_version"] = exportTable("database_version", {"version"});
    root["users"] = exportTable("users", {"uuid", "username", "email", "refreshToken"});
    root["categories"] =
        exportTable("categories", {"id", "uuid", "name", "user_uuid", "created_at", "updated_at", "synced"});
    root["todos"] =
        exportTable("todos", {"id", "uuid", "user_uuid", "title", "description", "category", "important", "deadline",
                              "recurrence_interval", "recurrence_count", "recurrence_start_date", "is_completed",
                              "completed_at", "is_deleted", "deleted_at", "created_at", "updated_at", "synced"});

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
    return true;
}

/**
 * @brief 从JSON文件导入数据库
 */
bool Database::importDatabaseFromJsonFile(const QString &filePath, bool replaceAll) {
    std::lock_guard<std::mutex> locker(m_mutex);

    if (!isDatabaseOpen()) {
        m_lastError = "数据库未打开";
        return false;
    }

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
    QJsonObject root = doc.object();

    auto getArray = [&](const char *key) -> QJsonArray {
        return root.contains(key) ? root.value(key).toArray() : QJsonArray();
    };

    QSqlQuery q(m_database);
    if (!m_database.transaction()) {
        m_lastError = "开启事务失败";
        return false;
    }

    auto rollback = [&]() {
        if (!m_database.rollback()) {
            qWarning() << "事务回滚失败";
        }
    };

    // 替换模式：清空表
    if (replaceAll) {
        const QStringList tables = {"todos", "categories", "users", "database_version"};
        for (const auto &t : tables) {
            if (!q.exec("DELETE FROM " + t)) {
                m_lastError = QString("清空表失败 %1: %2").arg(t, q.lastError().text());
                qCritical() << m_lastError;
                rollback();
                return false;
            }
        }
    }

    // 导入 users
    for (const auto &v : getArray("users")) {
        QJsonObject o = v.toObject();
        q.prepare("INSERT OR REPLACE INTO users (uuid, username, email, refreshToken) VALUES (?, ?, ?, ?)");
        q.addBindValue(o.value("uuid").toString());
        q.addBindValue(o.value("username").toString());
        q.addBindValue(o.value("email").toString());
        q.addBindValue(o.value("refreshToken").toString());
        if (!q.exec()) {
            m_lastError = QString("导入users失败: %1").arg(q.lastError().text());
            qCritical() << m_lastError;
            rollback();
            return false;
        }
    }

    // 导入 categories
    for (const auto &v : getArray("categories")) {
        QJsonObject o = v.toObject();
        q.prepare("INSERT OR REPLACE INTO categories (id, uuid, name, user_uuid, created_at, updated_at, synced) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)");
        q.addBindValue(o.value("id").toVariant());
        q.addBindValue(o.value("uuid").toString());
        q.addBindValue(o.value("name").toString());
        q.addBindValue(o.value("user_uuid").toString());
        q.addBindValue(o.value("created_at").toString());
        q.addBindValue(o.value("updated_at").toString());
        q.addBindValue(o.value("synced").toInt());
        if (!q.exec()) {
            m_lastError = QString("导入categories失败: %1").arg(q.lastError().text());
            qCritical() << m_lastError;
            rollback();
            return false;
        }
    }

    // 导入 todos
    for (const auto &v : getArray("todos")) {
        QJsonObject o = v.toObject();
        q.prepare(
            "INSERT OR REPLACE INTO todos (id, uuid, user_uuid, title, description, category, important, deadline, "
            "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, is_deleted, "
            "deleted_at, created_at, updated_at, synced) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        q.addBindValue(o.value("id").toVariant());
        q.addBindValue(o.value("uuid").toString());
        q.addBindValue(o.value("user_uuid").toString());
        q.addBindValue(o.value("title").toString());
        q.addBindValue(o.value("description").toVariant());
        q.addBindValue(o.value("category").toString());
        q.addBindValue(o.value("important").toInt());
        q.addBindValue(o.value("deadline").toVariant());
        q.addBindValue(o.value("recurrence_interval").toInt());
        q.addBindValue(o.value("recurrence_count").toInt());
        q.addBindValue(o.value("recurrence_start_date").toVariant());
        q.addBindValue(o.value("is_completed").toInt());
        q.addBindValue(o.value("completed_at").toVariant());
        q.addBindValue(o.value("is_deleted").toInt());
        q.addBindValue(o.value("deleted_at").toVariant());
        q.addBindValue(o.value("created_at").toString());
        q.addBindValue(o.value("updated_at").toString());
        q.addBindValue(o.value("synced").toInt());
        if (!q.exec()) {
            m_lastError = QString("导入todos失败: %1").arg(q.lastError().text());
            qCritical() << m_lastError;
            rollback();
            return false;
        }
    }

    // 导入 database_version（可选，没有则保持不变）
    QJsonArray verArr = getArray("database_version");
    if (!verArr.isEmpty()) {
        int ver = verArr.first().toObject().value("version").toInt(DATABASE_VERSION);
        if (!updateDatabaseVersion(ver)) {
            rollback();
            return false;
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
 * @brief 创建数据库表
 * @return 创建是否成功
 */
bool Database::createTables() {
    // 创建版本信息表
    if (!createVersionTable()) {
        return false;
    }

    // 创建用户表
    if (!createUsersTable()) {
        return false;
    }

    // 创建categories表
    if (!createCategoriesTable()) {
        return false;
    }

    // 创建todos表
    if (!createTodosTable()) {
        return false;
    }

    // 设置数据库版本
    if (!updateDatabaseVersion(DATABASE_VERSION)) {
        return false;
    }

    return true;
}

/**
 * @brief 创建用户表
 * @return 创建是否成功
 */
bool Database::createUsersTable() {
    const QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS users (
            uuid TEXT PRIMARY KEY NOT NULL,
            username TEXT NOT NULL,
            email TEXT NOT NULL,
            refreshToken TEXT NOT NULL
        )
    )";

    QSqlQuery query(m_database);
    if (!query.exec(createTableQuery)) {
        m_lastError = QString("创建用户表失败: %1").arg(query.lastError().text());
        qCritical() << m_lastError;
        return false;
    }

    return true;
}

/**
 * @brief 创建categories表
 * @return 创建是否成功
 */
bool Database::createCategoriesTable() {
    const QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            uuid TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            user_uuid TEXT NOT NULL,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            synced INTEGER NOT NULL DEFAULT 1
        )
    )";

    QSqlQuery query(m_database);
    if (!query.exec(createTableQuery)) {
        m_lastError = QString("创建categories表失败: %1").arg(query.lastError().text());
        qCritical() << m_lastError;
        return false;
    }

    // 创建索引
    const QStringList indexes = //
        {"CREATE INDEX IF NOT EXISTS idx_categories_uuid ON categories(uuid)",
         "CREATE INDEX IF NOT EXISTS idx_categories_user_uuid ON categories(user_uuid)",
         "CREATE INDEX IF NOT EXISTS idx_categories_name ON categories(name)"};

    for (const QString &indexQuery : indexes) {
        if (!query.exec(indexQuery)) {
            qWarning() << "创建categories表索引失败:" << query.lastError().text();
        }
    }

    return true;
}

/**
 * @brief 创建todos表
 * @return 创建是否成功
 */
bool Database::createTodosTable() {
    const QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS todos (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            uuid TEXT UNIQUE NOT NULL,
            user_uuid TEXT NOT NULL,
            title TEXT NOT NULL,
            description TEXT,
            category TEXT NOT NULL DEFAULT '未分类',
            important INTEGER NOT NULL DEFAULT 0,
            deadline TEXT,
            recurrence_interval INTEGER NOT NULL DEFAULT 0,
            recurrence_count INTEGER NOT NULL DEFAULT 0,
            recurrence_start_date TEXT,
            is_completed INTEGER NOT NULL DEFAULT 0,
            completed_at TEXT,
            is_deleted INTEGER NOT NULL DEFAULT 0,
            deleted_at TEXT,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            synced INTEGER NOT NULL DEFAULT 1
        )
    )";

    QSqlQuery query(m_database);
    if (!query.exec(createTableQuery)) {
        m_lastError = QString("创建todos表失败: %1").arg(query.lastError().text());
        qCritical() << m_lastError;
        return false;
    }

    // 创建索引
    const QStringList indexes = //
        {"CREATE INDEX IF NOT EXISTS idx_todos_uuid ON todos(uuid)",
         "CREATE INDEX IF NOT EXISTS idx_todos_user_uuid ON todos(user_uuid)",
         "CREATE INDEX IF NOT EXISTS idx_todos_category ON todos(category)",
         "CREATE INDEX IF NOT EXISTS idx_todos_deadline ON todos(deadline)",
         "CREATE INDEX IF NOT EXISTS idx_todos_completed ON todos(is_completed)",
         "CREATE INDEX IF NOT EXISTS idx_todos_deleted ON todos(is_deleted)",
         "CREATE INDEX IF NOT EXISTS idx_todos_synced ON todos(synced)"};

    for (const QString &indexQuery : indexes) {
        if (!query.exec(indexQuery)) {
            qWarning() << "创建todos表索引失败:" << query.lastError().text();
        }
    }

    return true;
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
