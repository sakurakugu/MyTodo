/**
 * @file database.h
 * @brief Database类的头文件
 *
 * 该文件定义了Database类，用于管理应用程序的SQLite数据库连接和初始化。
 *
 * @author Sakurakugu
 * @date 2025-09-15 20:55:22(UTC+8) 周一
 * @change 2025-09-22 19:36:18(UTC+8) 周一
 */

#pragma once

#include "default_value.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QStandardPaths>
#include <QDir>
#include <map>
#include <memory>
#include <mutex>

/**
 * @brief 数据导入导出接口
 *
 * 各个数据管理类（如UserAuth、TodoDataStorage、CategoryDataStorage）
 * 需要实现此接口以支持统一的导入导出功能
 */
class IDataExporter {
  public:
    virtual ~IDataExporter() = default;

    /**
     * @brief 导出数据到JSON对象
     * @param output 输出的JSON对象，各类将自己的数据填入对应的键
     * @return 导出是否成功
     */
    virtual bool 导出到JSON(QJsonObject &output) = 0;

    /**
     * @brief 从JSON对象导入数据
     * @param input 输入的JSON对象
     * @param replaceAll 是否替换所有数据
     * @return 导入是否成功
     */
    virtual bool 导入从JSON(const QJsonObject &input, bool replaceAll) = 0;
};

/**
 * @class Database
 * @brief 数据库管理器，负责SQLite数据库的连接和初始化
 *
 * Database类专门处理数据库相关的操作：
 *
 * **核心功能：**
 * - SQLite数据库连接管理
 * - 数据库表结构初始化
 * - 数据库版本管理和迁移
 * - 线程安全的数据库操作
 *
 * **设计原则：**
 * - 单例模式：确保全局唯一的数据库连接
 * - 线程安全：支持多线程环境下的数据库操作
 * - 自动初始化：首次使用时自动创建数据库和表结构
 *
 * **使用场景：**
 * - CategoryDataStorage和TodoDataStorage需要数据库连接时
 * - 应用程序启动时初始化数据库
 * - 数据库版本升级和迁移
 *
 * @note 该类是线程安全的，使用QMutex保护数据库操作
 * @see CategoryDataStorage, TodoDataStorage
 */
class Database : public QObject {
    Q_OBJECT

  public:
    // 单例模式
    static Database &GetInstance() {
        static Database instance;
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    Database(Database &&) = delete;
    Database &operator=(Database &&) = delete;

    // 数据库连接管理
    bool initializeDatabase();   ///< 初始化数据库连接和表结构
    QSqlDatabase getDatabase();  ///< 获取数据库连接
    bool isDatabaseOpen() const; ///< 检查数据库是否已打开
    void closeDatabase();        ///< 关闭数据库连接

    // 数据库操作
    bool executeQuery(const QString &queryString, QSqlQuery &query); ///< 执行SQL查询
    bool executeQuery(const QString &queryString);                   ///< 执行SQL查询（无返回结果）

    // 错误处理
    QString getLastError() const; ///< 获取最后一次错误信息

    // 数据库信息
    QString getDatabasePath() const; ///< 获取数据库文件路径
    int getDatabaseVersion() const;  ///< 获取数据库版本
    QString getSqliteVersion();      ///< 查询sqlite版本

    // 数据导出器管理
    void registerDataExporter(const QString &name, IDataExporter *exporter); ///< 注册数据导出器
    void unregisterDataExporter(const QString &name);                        ///< 注销数据导出器

    // 通用的数据库导入导出接口
    bool exportDataToJson(QJsonObject &output);                         ///< 导出所有表数据到JSON对象
    bool importDataFromJson(const QJsonObject &input, bool replaceAll); ///< 从JSON对象导入数据到数据库

    bool exportDatabaseToJsonFile(const QString &filePath);                    ///< 导出数据库所有表到JSON文件
    bool importDatabaseFromJsonFile(const QString &filePath, bool replaceAll); ///< 从JSON文件导入数据库

  private:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    // 数据库初始化相关
    bool createVersionTable(); ///< 创建版本信息表

    // 数据库迁移
    bool migrateDatabase(int fromVersion, int toVersion); ///< 数据库版本迁移
    bool updateDatabaseVersion(int version);              ///< 更新数据库版本

    // 辅助方法
    bool tableExists(const QString &tableName);                               ///< 检查表是否存在
    QJsonArray exportTable(const QString &table, const QStringList &columns); ///< 导出指定表到JSON数组

    // 成员变量
    mutable std::mutex m_mutex; ///< 线程安全保护
    QSqlDatabase m_database;    ///< 数据库连接对象
    QString m_lastError;        ///< 最后一次错误信息
    bool m_initialized;         ///< 是否已初始化
    QString m_databasePath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                                 .absoluteFilePath(QString(DefaultValues::appName) + ".db"); ///< 数据库文件路径

    // 数据导出器管理
    std::map<QString, IDataExporter *> m_dataExporters; ///< 注册的数据导出器

    static constexpr int DATABASE_VERSION = 1;                                                   ///< 当前数据库版本
    static inline const QString CONNECTION_NAME = QString(DefaultValues::appName) + "_Database"; ///< 数据库连接名称
};