/**
 * @file database.h
 * @brief Database类的头文件
 *
 * 该文件定义了Database类，用于管理应用程序的SQLite数据库连接和初始化。
 * 使用原生sqlite3.h而非Qt的QSql框架，提供轻量级的数据库管理功能。
 *
 * @author Sakurakugu
 * @date 2025-09-15 20:55:22 (UTC+8) 周一
 * @change 2025-10-05 21:25:00 (UTC+8) 周日
 */

#pragma once

#include "sql_query.h"
#include "version.h"

#include <functional>
#include <memory>
#include <mutex>
#include <sqlite3.h>
#include <string>
#include <vector>

#ifdef QT_CORE_LIB
#include <QJsonObject>
#include <QString>
#endif

/**
 * @brief 数据导入导出接口（原生SQLite版本）
 *
 * 各个数据管理类需要实现此接口以支持统一的导入导出功能
 */
class IDataExporter {
  public:
    virtual ~IDataExporter() = default;

    /**
     * @brief 导出数据到JSON对象
     * @param output 输出的JSON对象，各类将自己的数据填入对应的键
     * @return 导出是否成功
     */
    virtual bool exportToJson(QJsonObject &output) = 0;

    /**
     * @brief 从JSON对象导入数据
     * @param input 输入的JSON对象
     * @param replaceAll 是否替换所有数据
     * @return 导入是否成功
     */
    virtual bool importFromJson(const QJsonObject &input, bool replaceAll) = 0;
};

/**
 * @class Database
 * @brief 原生SQLite数据库管理器，负责SQLite数据库的连接和初始化
 *
 * Database类专门处理基于sqlite3.h的数据库相关操作：
 *
 * **核心功能：**
 * - 原生SQLite数据库连接管理
 * - 数据库表结构初始化
 * - 数据库版本管理和迁移
 * - 线程安全的数据库操作
 * - 事务管理
 * - 连接池管理
 *
 * **设计原则：**
 * - 单例模式：确保全局唯一的数据库连接管理
 * - 线程安全：支持多线程环境下的数据库操作
 * - 自动初始化：首次使用时自动创建数据库和表结构
 * - 轻量级：使用原生sqlite3.h，避免Qt依赖
 *
 * **使用场景：**
 * - 需要高性能数据库操作的场景
 * - 避免Qt依赖的轻量级应用
 * - 需要精细控制SQLite特性的场景
 *
 * @note 该类是线程安全的，使用std::mutex保护数据库操作
 * @see SqlQuery
 */
class Database {
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
    bool initialize();                   ///< 初始化数据库连接和表结构
    bool isInitialized() const;          ///< 检查是否已初始化
    void close();                        ///< 关闭数据库连接
    sqlite3 *getHandle();                ///< 获取数据库句柄
    std::string getDatabasePath() const; ///< 获取数据库文件路径

    // 数据库操作
    std::unique_ptr<SqlQuery> createQuery();                       ///< 创建查询对象
    std::unique_ptr<SqlQuery> createQuery(const std::string &sql); ///< 创建并准备查询对象

    // 事务管理
    bool beginTransaction();    ///< 开始事务
    bool commitTransaction();   ///< 提交事务
    bool rollbackTransaction(); ///< 回滚事务

    // 事务RAII包装器
    class Transaction {
      public:
        explicit Transaction(Database &db);
        ~Transaction();
        bool commit();   ///< 手动提交事务
        bool rollback(); ///< 手动回滚事务

      private:
        Database &m_db;
        bool m_committed;
        bool m_active;
    };

    // 数据库信息
    int getDatabaseVersion() const; ///< 获取数据库版本
    std::string getSqliteVersion(); ///< 获取SQLite版本
    std::string lastError() const;  ///< 获取最后一次错误信息
    bool hasError() const;          ///< 检查是否有错误

    // 表管理
    bool tableExists(const std::string &tableName);                                 ///< 检查表是否存在
    std::vector<std::string> getTableNames();                                       ///< 获取所有表名
    std::vector<std::string> getColumnNames(const std::string &tableName);          ///< 获取表的列名
    bool createTable(const std::string &tableName, const std::string &tableSchema); ///< 创建表
    bool dropTable(const std::string &tableName);                                   ///< 删除表

    // 数据导入导出
#ifdef QT_CORE_LIB
    void registerDataExporter(const std::string &name, IDataExporter *exporter); ///< 注册数据导出器
    void unregisterDataExporter(const std::string &name);                        ///< 注销数据导出器
    bool exportDataToJson(QJsonObject &output);                                  ///< 导出所有数据到JSON对象
    bool importDataFromJson(const QJsonObject &input, bool replaceAll);          ///< 从JSON对象导入数据
    bool exportToJsonFile(const std::string &filePath);                          ///< 导出数据到JSON文件
    bool importFromJsonFile(const std::string &filePath, bool replaceAll);       ///< 从JSON文件导入数据
#endif

    // 数据库优化
    bool vacuum();                                                       ///< 压缩数据库
    bool analyze();                                                      ///< 分析数据库统计信息
    bool enableWALMode();                                                ///< 启用WAL模式
    bool setPragma(const std::string &pragma, const std::string &value); ///< 设置PRAGMA

  private:
    explicit Database();
    ~Database();

    // 内部初始化方法
    bool createVersionTable();                            ///< 创建版本信息表
    bool migrateDatabase(int fromVersion, int toVersion); ///< 数据库版本迁移
    bool updateDatabaseVersion(int version);              ///< 更新数据库版本
    bool setupDatabase();                                 ///< 设置数据库配置

    // 内部方法（不加锁）
    sqlite3 *getHandleInternal();                                          ///< 获取数据库句柄（内部使用，不加锁）
    std::unique_ptr<SqlQuery> createQueryInternal();                       ///< 创建查询对象（内部使用，不加锁）
    std::unique_ptr<SqlQuery> createQueryInternal(const std::string &sql); ///< 创建并准备查询对象（内部使用，不加锁）
    bool setPragmaInternal(const std::string &pragma, const std::string &value); ///< 设置PRAGMA（内部使用，不加锁）

    // 错误处理
    void setError(const std::string &error); ///< 设置错误信息
    void clearError();                       ///< 清除错误状态

    // 成员变量
    mutable std::mutex m_mutex;      ///< 线程安全保护
    sqlite3 *m_db;                   ///< 数据库句柄
    std::string m_databasePath;      ///< 数据库文件路径
    std::string m_lastError;         ///< 最后一次错误信息
    std::atomic<bool> m_initialized; ///< 是否已初始化
    std::atomic<bool> m_hasError;    ///< 是否有错误
    int m_transactionLevel;          ///< 事务嵌套级别

#ifdef QT_CORE_LIB
    std::map<std::string, IDataExporter *> m_dataExporters;                   ///< 注册的数据导出器
    QJsonArray exportTable(const QString &table, const QStringList &columns); ///< 导出指定表到JSON数组
#endif

    // 常量
    static constexpr int DATABASE_VERSION = 1;                                         ///< 当前数据库版本
    static inline const std::string DATABASE_FILENAME = std::string(APP_NAME) + ".db"; ///< 数据库文件名
};