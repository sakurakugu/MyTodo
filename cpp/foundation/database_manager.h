/**
 * @file database_manager.h
 * @brief DatabaseManager类的头文件
 *
 * 该文件定义了DatabaseManager类，用于管理应用程序的SQLite数据库连接和初始化。
 *
 * @author Sakurakugu
 * @date 2025-01-27 00:00:00(UTC+8) 周一
 * @change 2025-01-27 00:00:00(UTC+8) 周一
 * @version 0.4.0
 */

#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QMutex>
#include <memory>

/**
 * @class DatabaseManager
 * @brief 数据库管理器，负责SQLite数据库的连接和初始化
 *
 * DatabaseManager类专门处理数据库相关的操作：
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
class DatabaseManager : public QObject {
    Q_OBJECT

public:
    // 单例模式
    static DatabaseManager &GetInstance() {
        static DatabaseManager instance;
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;
    DatabaseManager(DatabaseManager &&) = delete;
    DatabaseManager &operator=(DatabaseManager &&) = delete;

    // 数据库连接管理
    bool initializeDatabase();                    ///< 初始化数据库连接和表结构
    QSqlDatabase getDatabase();                   ///< 获取数据库连接
    bool isDatabaseOpen() const;                  ///< 检查数据库是否已打开
    void closeDatabase();                         ///< 关闭数据库连接
    
    // 数据库操作
    bool executeQuery(const QString &queryString, QSqlQuery &query); ///< 执行SQL查询
    bool executeQuery(const QString &queryString);                   ///< 执行SQL查询（无返回结果）
    
    // 错误处理
    QString getLastError() const;                 ///< 获取最后一次错误信息
    
    // 数据库信息
    QString getDatabasePath() const;              ///< 获取数据库文件路径
    int getDatabaseVersion() const;               ///< 获取数据库版本
    
private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();
    
    // 数据库初始化相关
    bool createTables();                          ///< 创建数据库表
    bool createCategoriesTable();                 ///< 创建categories表
    bool createTodosTable();                      ///< 创建todos表
    bool createVersionTable();                    ///< 创建版本信息表
    
    // 数据库迁移
    bool migrateDatabase(int fromVersion, int toVersion); ///< 数据库版本迁移
    bool updateDatabaseVersion(int version);      ///< 更新数据库版本
    
    // 辅助方法
    QString getDefaultDatabasePath() const;       ///< 获取默认数据库路径
    bool tableExists(const QString &tableName);   ///< 检查表是否存在
    
    // 成员变量
    mutable QMutex m_mutex;                       ///< 线程安全保护
    QSqlDatabase m_database;                      ///< 数据库连接对象
    QString m_databasePath;                       ///< 数据库文件路径
    QString m_lastError;                          ///< 最后一次错误信息
    bool m_initialized;                           ///< 是否已初始化
    
    static constexpr int DATABASE_VERSION = 1;    ///< 当前数据库版本
    static constexpr const char* CONNECTION_NAME = "MyTodoDatabase"; ///< 数据库连接名称
};