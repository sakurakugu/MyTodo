/**
 * @file sql_query.h
 * @brief SqlQuery类的头文件
 *
 * 该文件定义了SqlQuery类，用于执行原生SQLite查询操作，使用sqlite3.h而非Qt的QSql。
 * 从Database类中拆分出SQL查询相关的功能，提供更轻量级的数据库查询接口。
 *
 * @author Sakurakugu
 * @date 2025-10-09 23:37 (UTC+8)
 */

#pragma once

#include <functional>
#include <memory>
#include <sqlite3.h>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#ifdef QT_CORE_LIB
#include <QDateTime>
#include <QString>
#include <QUuid>
#endif

/**
 * @brief SQL值类型定义
 * 支持多种数据类型的SQL值，用于参数绑定和结果获取
 * @note 若要新增类型，记得在bindValue中也添加对应的处理
 */
using SqlValue = std::variant< //
    int32_t,                   // 32位整数
    int64_t,                   // 64位整数
    double,                    // 双精度浮点数
    bool,                      // 布尔值
    std::string,               // 标准字符串
    const char *,              // C字符串
    std::vector<uint8_t>,      // 二进制数据(BLOB)
    std::nullptr_t             // NULL值
#ifdef QT_CORE_LIB
    ,
    QString,  // Qt字符串
    QUuid,    // Qt UUID
    QDateTime // Qt日期时间
#endif
    >;

/**
 * @brief 查询结果行类型
 * 表示查询结果的一行数据，使用列名作为键
 */
using SqlRow = std::vector<std::pair<std::string, SqlValue>>;
using SqlMap = std::map<std::string, SqlValue>;

/**
 * @brief 查询结果集类型
 * 表示完整的查询结果，包含多行数据
 */
using SqlResultSet = std::vector<SqlRow>;
using SqlMapResultSet = std::vector<SqlMap>;

/**
 * @class SqlQuery
 * @brief 原生SQLite查询类
 *
 * SqlQuery类提供基于sqlite3.h的数据库查询功能：
 *
 * **核心功能：**
 * - 原生SQLite查询执行
 * - SQL参数绑定和转义
 * - 结果集获取和处理
 * - 预编译语句支持
 *
 * **设计原则：**
 * - 轻量级：不依赖Qt的QSql框架
 * - 类型安全：使用std::variant支持多种数据类型
 * - 防注入：提供SQL转义和参数绑定功能
 * - 高性能：支持预编译语句重复使用
 *
 * **使用场景：**
 * - 需要原生SQLite性能的场景
 * - 不依赖Qt框架的模块
 * - 复杂SQL查询和批量操作
 *
 * @note 该类不是线程安全的，需要外部同步
 */
class SqlQuery {
  public:
    explicit SqlQuery(sqlite3 *db_handle);
    ~SqlQuery();

    // 禁用拷贝构造和赋值
    SqlQuery(const SqlQuery &) = delete;
    SqlQuery &operator=(const SqlQuery &) = delete;

    // 支持移动构造和赋值
    SqlQuery(SqlQuery &&other) noexcept;
    SqlQuery &operator=(SqlQuery &&other) noexcept;

    bool prepare(const std::string &sql);                                           // 准备SQL语句
    bool bindValue(int index, const SqlValue &value);                               // 绑定参数到预编译语句
    bool bindValues(const std::vector<SqlValue> &values);                           // 绑定多个参数到预编译语句
    template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 1)>> //
    bool bindValues(Args &&...args) {
        std::vector<SqlValue> values = {SqlValue(std::forward<Args>(args))...};
        return bindValues(values);
    }

    // 自动索引绑定方法
    bool addBindValue(const SqlValue &value);                                       // 自动绑定参数到下一个位置
    bool addBindValues(const std::vector<SqlValue> &values);                        // 自动绑定多个参数
    template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 1)>> //
    bool addBindValues(Args &&...args) {
        std::vector<SqlValue> values = {SqlValue(std::forward<Args>(args))...};
        return addBindValues(values);
    }

    bool exec();                                                                    // 执行预编译语句
    bool exec(const std::string &sql);                                              // 执行SQL语句（一次性）
    bool exec(const std::string &sqlTemplate, const std::vector<SqlValue> &values); // 执行带参数的SQL语句
    template <typename... Args> bool exec(const std::string &sqlTemplate, Args &&...args) {
        std::vector<SqlValue> values = {SqlValue(std::forward<Args>(args))...};
        return exec(sqlTemplate, values);
    }

    bool next();                                         // 移动到下一行结果
    SqlValue value(int index) const;                     // 获取当前行的列值
    SqlValue value(const std::string &columnName) const; // 获取当前行的列值
    SqlRow currentRow() const;                           // 获取当前行的所有数据
    SqlMap currentMap() const;                           // 获取当前行的所有数据（映射）
    SqlResultSet fetchAll();                             // 获取所有结果行
    SqlMapResultSet fetchAllMap();                       // 获取所有结果行（映射）
    bool reset();                                        // 重置语句状态
    bool clearBindings();                                // 清理绑定的参数
    int columnCount() const;                             // 获取列数量
    std::string columnName(int index) const;             // 获取列名
    int rowsAffected() const;                            // 获取受影响的行数
    int64_t lastInsertRowId() const;                     // 获取最后插入的行ID
    std::string lastError() const;                       // 获取最后的错误信息
    bool hasError() const;                               // 检查是否有错误
    int parameterCount() const;                          // 获取参数数量

  private:
    sqlite3 *m_db;           ///< 数据库句柄
    sqlite3_stmt *m_stmt;    ///< 预编译语句句柄
    std::string m_lastError; ///< 最后的错误信息
    bool m_hasError;         ///< 是否有错误
    int m_bindIndex;         ///< 当前绑定索引（从1开始）

    void setError(const std::string &error);                           // 设置错误信息
    void clearError();                                                 // 清除错误状态
    static SqlValue valueFromSqlite(sqlite3_value *value);             // 从sqlite3_value获取SqlValue
    static SqlValue valueFromStatement(sqlite3_stmt *stmt, int index); // 从当前语句结果获取SqlValue
};
