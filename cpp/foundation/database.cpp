/**
 * @file database.cpp
 * @brief DatabaseManager类的实现
 *
 * 该文件实现了DatabaseManager类，用于管理应用程序的SQLite数据库连接和初始化。
 *
 * @author Sakurakugu
 * @date 2025-01-27 00:00:00(UTC+8) 周一
 * @change 2025-01-27 00:00:00(UTC+8) 周一
 * @version 0.4.0
 */

#include "database.h"
#include "config.h"
#include "version.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>
#include <mutex>

Database::Database(QObject *parent) : QObject(parent), m_initialized(false) {
    m_databasePath =
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).absoluteFilePath("MyTodo.db");
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
    qInfo() << "数据库初始化成功:" << m_databasePath;
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
            accessToken TEXT NOT NULL,
            refreshToken TEXT NOT NULL,
            tokenExpiryTime INTEGER NOT NULL
        )
    )";

    QSqlQuery query(m_database);
    if (!query.exec(createTableQuery)) {
        m_lastError = QString("创建用户表失败: %1").arg(query.lastError().text());
        qCritical() << m_lastError;
        return false;
    }

    qDebug() << "用户表创建成功";
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
            last_modified_at TEXT NOT NULL,
            synced INTEGER NOT NULL DEFAULT 0
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

    qDebug() << "categories表创建成功";
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
            last_modified_at TEXT NOT NULL,
            synced INTEGER NOT NULL DEFAULT 0
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

    qDebug() << "todos表创建成功";
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

    qDebug() << "database_version表创建成功";
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

    qInfo() << "数据库版本已更新为:" << version;
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