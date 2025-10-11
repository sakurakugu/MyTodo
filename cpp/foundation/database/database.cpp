/**
 * @file database.cpp
 * @brief Database类实现文件
 *
 * 实现基于sqlite3.h的SQLite数据库连接生命周期管理、版本迁移、线程安全封装、
 * 统一数据导入/导出接口注册与调度等功能。
 *
 * @author Sakurakugu
 * @date 2025-09-15 20:55:22 (UTC+8) 周一
 * @change 2025-10-05 21:25:00 (UTC+8) 周日
 */

#include "database.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#ifdef QT_CORE_LIB
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#endif

Database::Database()
    : m_db(nullptr),        //
      m_initialized(false), //
      m_hasError(false),    //
      m_transactionLevel(0) {
#ifdef QT_CORE_LIB
    // 使用Qt路径
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDir(appDataPath);
    if (!appDir.exists()) {
        appDir.mkpath(".");
    }
    m_databasePath = appDir.absoluteFilePath(QString::fromStdString(DATABASE_FILENAME)).toStdString();
    qInfo() << "数据库路径:" << m_databasePath;
#else
    // 使用标准C++路径
    std::filesystem::path appDataPath;
#ifdef _WIN32
    const char *appData = std::getenv("APPDATA");
    if (appData) {
        appDataPath = std::filesystem::path(appData) / APP_NAME;
    } else {
        appDataPath = std::filesystem::current_path();
    }
#else
    const char *home = std::getenv("HOME");
    if (home) {
        appDataPath = std::filesystem::path(home) / ".local" / "share" / APP_NAME;
    } else {
        appDataPath = std::filesystem::current_path();
    }
#endif

    // 确保目录存在
    std::filesystem::create_directories(appDataPath);
    m_databasePath = (appDataPath / DATABASE_FILENAME).string();
#endif
}

Database::~Database() {
    close();
}

bool Database::initialize() {
    if (m_initialized.load(std::memory_order_acquire))
        return true;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized.store(true, std::memory_order_release);

    clearError();

    // 打开数据库
    int result = sqlite3_open(m_databasePath.c_str(), &m_db);
    if (result != SQLITE_OK) {
        setError("无法打开数据库: " + std::string(sqlite3_errmsg(m_db)));
        if (m_db) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        m_initialized.store(false, std::memory_order_release);
        return false;
    }

    // 设置数据库配置
    if (!setupDatabase()) {
        close();
        m_initialized.store(false, std::memory_order_release);
        return false;
    }

    // 创建版本信息表
    if (!createVersionTable()) {
        close();
        m_initialized.store(false, std::memory_order_release);
        return false;
    }
    // 设置数据库版本
    if (!updateDatabaseVersion(DATABASE_VERSION)) {
        close();
        m_initialized.store(false, std::memory_order_release);
        return false;
    }

    m_initialized.store(true, std::memory_order_release);

#ifdef QT_CORE_LIB
    qInfo() << "数据库初始化成功:" << m_databasePath << "版本:" << DATABASE_VERSION;
#else
    std::cout << "数据库初始化成功: " << m_databasePath << " 版本: " << DATABASE_VERSION << std::endl;
#endif

    return true;
}

bool Database::isInitialized() const {
    return m_initialized.load(std::memory_order_acquire) && m_db != nullptr;
}

void Database::close() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_db) {
        // 回滚所有未提交的事务
        while (m_transactionLevel > 0) {
            sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, nullptr);
            m_transactionLevel--;
        }

        sqlite3_close(m_db);
        m_db = nullptr;
    }

    m_initialized = false;
    clearError();
}

sqlite3 *Database::getHandle() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return getHandleInternal();
}

sqlite3 *Database::getHandleInternal() {
    if (!isInitialized()) {
        setError("数据库未初始化");
        return nullptr;
    }

    return m_db;
}

std::string Database::getDatabasePath() const {
    return m_databasePath;
}

std::unique_ptr<SqlQuery> Database::createQuery() {
    std::lock_guard<std::mutex> lock(m_mutex);
    sqlite3 *handle = getHandleInternal();
    if (!handle) {
        return nullptr;
    }

    return std::make_unique<SqlQuery>(handle);
}

std::unique_ptr<SqlQuery> Database::createQuery(const std::string &sql) {
    auto query = createQuery();
    if (!query) {
        return nullptr;
    }

    if (!query->prepare(sql)) {
        setError(query->lastError());
        return nullptr;
    }

    return query;
}

std::unique_ptr<SqlQuery> Database::createQueryInternal() {
    sqlite3 *handle = getHandleInternal();
    if (!handle) {
        return nullptr;
    }

    return std::make_unique<SqlQuery>(handle);
}

std::unique_ptr<SqlQuery> Database::createQueryInternal(const std::string &sql) {
    auto query = createQueryInternal();
    if (!query) {
        return nullptr;
    }

    if (!query->prepare(sql)) {
        setError(query->lastError());
        return nullptr;
    }

    return query;
}

bool Database::beginTransaction() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized || !m_db) {
        setError("数据库未初始化");
        return false;
    }

    clearError();

    // 支持嵌套事务（使用保存点）
    std::string sql;
    if (m_transactionLevel == 0) {
        sql = "BEGIN TRANSACTION";
    } else {
        sql = "SAVEPOINT sp" + std::to_string(m_transactionLevel);
    }

    char *errorMsg = nullptr;
    int result = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errorMsg);

    if (result != SQLITE_OK) {
        std::string error = "开始事务失败: ";
        if (errorMsg) {
            error += errorMsg;
            sqlite3_free(errorMsg);
        } else {
            error += sqlite3_errmsg(m_db);
        }
        setError(error);
        return false;
    }

    m_transactionLevel++;
    return true;
}

bool Database::commitTransaction() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized || !m_db) {
        setError("数据库未初始化");
        return false;
    }

    if (m_transactionLevel <= 0) {
        setError("没有活动的事务");
        return false;
    }

    clearError();

    std::string sql;
    if (m_transactionLevel == 1) {
        sql = "COMMIT TRANSACTION";
    } else {
        sql = "RELEASE SAVEPOINT sp" + std::to_string(m_transactionLevel - 1);
    }

    char *errorMsg = nullptr;
    int result = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errorMsg);

    if (result != SQLITE_OK) {
        std::string error = "提交事务失败: ";
        if (errorMsg) {
            error += errorMsg;
            sqlite3_free(errorMsg);
        } else {
            error += sqlite3_errmsg(m_db);
        }
        setError(error);
        return false;
    }

    m_transactionLevel--;
    return true;
}

bool Database::rollbackTransaction() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized || !m_db) {
        setError("数据库未初始化");
        return false;
    }

    if (m_transactionLevel <= 0) {
        setError("没有活动的事务");
        return false;
    }

    clearError();

    std::string sql;
    if (m_transactionLevel == 1) {
        sql = "ROLLBACK TRANSACTION";
    } else {
        sql = "ROLLBACK TO SAVEPOINT sp" + std::to_string(m_transactionLevel - 1);
    }

    char *errorMsg = nullptr;
    int result = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errorMsg);

    if (result != SQLITE_OK) {
        std::string error = "回滚事务失败: ";
        if (errorMsg) {
            error += errorMsg;
            sqlite3_free(errorMsg);
        } else {
            error += sqlite3_errmsg(m_db);
        }
        setError(error);
        return false;
    }

    m_transactionLevel--;
    return true;
}

// Transaction RAII 包装器实现
Database::Transaction::Transaction(Database &db) : m_db(db), m_committed(false), m_active(false) {
    m_active = m_db.beginTransaction();
}

Database::Transaction::~Transaction() {
    if (m_active && !m_committed) {
        m_db.rollbackTransaction();
    }
}

bool Database::Transaction::commit() {
    if (!m_active || m_committed) {
        return false;
    }

    bool result = m_db.commitTransaction();
    if (result) {
        m_committed = true;
    }
    return result;
}

bool Database::Transaction::rollback() {
    if (!m_active || m_committed) {
        return false;
    }

    bool result = m_db.rollbackTransaction();
    if (result) {
        m_active = false;
    }
    return result;
}

int Database::getDatabaseVersion() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized || !m_db) {
        return -1;
    }

    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT version FROM database_version LIMIT 1";

    int result = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        return -1;
    }

    int version = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return version;
}

std::string Database::getSqliteVersion() {
    return sqlite3_libversion();
}

std::string Database::lastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

bool Database::hasError() const {
    return m_hasError.load(std::memory_order_acquire);
}

bool Database::vacuum() {
    auto query = createQuery("VACUUM");
    if (!query) {
        return false;
    }
    return query->exec();
}

bool Database::analyze() {
    auto query = createQuery("ANALYZE");
    if (!query) {
        return false;
    }
    return query->exec();
}

bool Database::enableWALMode() {
    return setPragma("journal_mode", "WAL");
}

bool Database::setPragma(const std::string &pragma, const std::string &value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return setPragmaInternal(pragma, value);
}

bool Database::setPragmaInternal(const std::string &pragma, const std::string &value) {
    std::string sql = "PRAGMA " + pragma + " = " + value;
    auto query = createQueryInternal(sql);
    if (!query) {
        return false;
    }
    return query->exec();
}

bool Database::createVersionTable() {
    const char *sql = R"(
        CREATE TABLE IF NOT EXISTS database_version (
            version INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    auto query = createQueryInternal(sql);
    if (!query) {
        return false;
    }
    return query->exec();
}

bool Database::migrateDatabase([[maybe_unused]] int fromVersion, [[maybe_unused]] int toVersion) {
    // 这里可以添加数据库迁移逻辑
    // 目前只是简单的版本更新
    return updateDatabaseVersion(toVersion);
}

bool Database::updateDatabaseVersion(int version) {
    // 首先检查是否已有版本记录
    auto query = createQueryInternal("INSERT INTO database_version (version) VALUES (?)");
    if (!query) {
        return false;
    }
    if (!query->bindValue(1, version)) {
        return false;
    }
    if (!query->exec()) {
        return false;
    }

    auto countValue = query->value(0);
    int count = 0;
    if (std::holds_alternative<int32_t>(countValue)) {
        count = std::get<int32_t>(countValue);
    } else if (std::holds_alternative<int64_t>(countValue)) {
        count = static_cast<int>(std::get<int64_t>(countValue));
    }

    std::string sql;
    if (count == 0) {
        sql = "INSERT INTO database_version (version) VALUES (?)";
    } else {
        sql = "UPDATE database_version SET version = ?, updated_at = CURRENT_TIMESTAMP WHERE 1=1";
    }

    query = createQueryInternal(sql);
    if (!query) {
        return false;
    }
    return query->bindValue(1, version) && query->exec();
}

bool Database::setupDatabase() {
    // 启用外键约束
    if (!setPragmaInternal("foreign_keys", "ON")) {
        setError("无法启用外键约束");
        return false;
    }

    // 设置同步模式为NORMAL（平衡性能和安全性）
    if (!setPragmaInternal("synchronous", "NORMAL")) {
        setError("无法设置同步模式");
        return false;
    }

    // 设置缓存大小（2MB）
    if (!setPragmaInternal("cache_size", "2000")) {
        setError("无法设置缓存大小");
        return false;
    }

    return true;
}

void Database::setError(const std::string &error) {
    m_lastError = error;
    m_hasError.store(true, std::memory_order_release);

#ifdef QT_CORE_LIB
    qCritical() << QString::fromStdString(error);
#else
    std::cerr << "Database Error: " << error << std::endl;
#endif
}

void Database::clearError() {
    m_lastError.clear();
    m_hasError.store(false, std::memory_order_release);
}

#ifdef QT_CORE_LIB
void Database::registerDataExporter(const std::string &name, IDataExporter *exporter) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (exporter) {
        m_dataExporters[name] = exporter;
    }
}

void Database::unregisterDataExporter(const std::string &name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dataExporters.erase(name);
}

bool Database::exportDataToJson(QJsonObject &output) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // output["meta"] =
    //     QJsonObject{{"version", QString(APP_VERSION_STRING)},
    //                 {"database_version", DATABASE_VERSION},
    //                 {"sqlite_version", getSqliteVersion()},
    //                 {"export_time", QDateTime::currentDateTime().toString(Qt::ISODate)}}; // 不是rfc3339格式

    // 还有数据库版本
    // output["database_version"] = exportTable("database_version", {"version"});

    for (const auto &[name, exporter] : m_dataExporters) {
        if (!exporter->exportToJson(output)) {
            setError("导出数据失败: " + name);
            return false;
        }
    }

    return true;
}

bool Database::importDataFromJson(const QJsonObject &input, bool replaceAll) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Transaction transaction(*this);

    for (const auto &[name, exporter] : m_dataExporters) {
        if (!exporter->importFromJson(input, replaceAll)) {
            setError("导入数据失败: " + name);
            return false;
        }
    }

    return transaction.commit();
}

bool Database::exportToJsonFile(const std::string &filePath) {
    QJsonObject output;
    if (!exportDataToJson(output)) {
        return false;
    }

    QJsonDocument doc(output);
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::WriteOnly)) {
        setError("无法打开文件进行写入: " + filePath);
        return false;
    }

    file.write(doc.toJson());
    return true;
}

bool Database::importFromJsonFile(const std::string &filePath, bool replaceAll) {
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly)) {
        setError("无法打开文件进行读取: " + filePath);
        return false;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        setError("JSON解析失败: " + error.errorString().toStdString());
        return false;
    }

    return importDataFromJson(doc.object(), replaceAll);
}

/**
 * @brief 导出指定表到JSON数组
 */
QJsonArray Database::exportTable(const QString &table, const QStringList &columns) {
    QJsonArray array;
    auto query = createQuery(std::format("SELECT {} FROM {}", columns.join(", ").toStdString(), table.toStdString()));
    if (!query || !query->exec()) {
        setError("导出表失败: " + table.toStdString() + " " + query->lastError());
        return array;
    }
    while (query->next()) {
        QJsonObject obj;
        for (int i = 0; i < columns.size(); ++i) {
            auto v = query->value(i);
            if (std::holds_alternative<bool>(v))
                obj[columns[i]] = sqlValueCast<bool>(v);
            // else if (std::holds_alternative<int32_t>(v) || std::holds_alternative<int64_t>(v))
            //     obj[columns[i]] = QJsonValue::fromVariant(v);
            else if (std::holds_alternative<double>(v))
                obj[columns[i]] = sqlValueCast<double>(v);
            // else if (std::holds_alternative<std::string>(v))
            //     obj[columns[i]] = QString::fromStdString(v);
            // else if (std::holds_alternative<QString>(v))
            //     obj[columns[i]] = v;
            else if (std::holds_alternative<QUuid>(v))
                obj[columns[i]] = QString::fromStdString(sqlValueCast<std::string>(v));
            else if (std::holds_alternative<QDateTime>(v))
                obj[columns[i]] = QString::fromStdString(sqlValueCast<std::string>(v));
            // else if (std::holds_alternative<std::vector<uint8_t>>(v))
            //     obj[columns[i]] =
            //     QJsonValue::fromVariant(QByteArray::fromStdVector(sqlValueCast<std::vector<uint8_t>>(v)));
            else if (std::holds_alternative<std::nullptr_t>(v))
                obj[columns[i]] = QJsonValue();
            // else
            //     obj[columns[i]] = QJsonValue::fromVariant(v);
        }
        array.append(obj);
    }
    return array;
}
#endif