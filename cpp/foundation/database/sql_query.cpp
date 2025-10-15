/**
 * @file sql_query.cpp
 * @brief SqlQuery类的实现文件
 *
 * 实现基于sqlite3.h的原生SQLite查询功能，包括参数绑定、结果获取、SQL转义等。
 *
 * @author Sakurakugu
 * @date 2025-10-09 23:37 (UTC+8)
 */

#include "sql_query.h"

/**
 * @brief 构造函数
 * @param db_handle SQLite数据库句柄
 */
SqlQuery::SqlQuery(sqlite3 *db_handle)
    : m_db(db_handle),   //
      m_stmt(nullptr),   //
      m_hasError(false), //
      m_bindIndex(1)     //
{
    if (!m_db)
        setError("无效的数据库句柄");
}

/**
 * @brief 析构函数
 */
SqlQuery::~SqlQuery() {
    if (m_stmt) {
        sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
    }
}

SqlQuery::SqlQuery(SqlQuery &&other) noexcept
    : m_db(other.m_db),                          //
      m_stmt(other.m_stmt),                      //
      m_lastError(std::move(other.m_lastError)), //
      m_hasError(other.m_hasError),              //
      m_bindIndex(other.m_bindIndex)             //
{
    other.m_db = nullptr;
    other.m_stmt = nullptr;
    other.m_hasError = false;
    other.m_bindIndex = 1;
}

SqlQuery &SqlQuery::operator=(SqlQuery &&other) noexcept {
    if (this != &other) {
        // 清理当前资源
        if (m_stmt) {
            sqlite3_finalize(m_stmt);
        }

        // 移动资源
        m_db = other.m_db;
        m_stmt = other.m_stmt;
        m_lastError = std::move(other.m_lastError);
        m_hasError = other.m_hasError;
        m_bindIndex = other.m_bindIndex;

        // 清空源对象
        other.m_db = nullptr;
        other.m_stmt = nullptr;
        other.m_hasError = false;
        other.m_bindIndex = 1;
    }
    return *this;
}

/**
 * @brief 准备SQL语句
 * @param sql SQL语句字符串
 * @return 准备是否成功
 */
bool SqlQuery::prepare(const std::string &sql) {
    clearError();

    if (!m_db) {
        setError("无效的数据库句柄");
        return false;
    }

    // 清理之前的语句
    if (m_stmt) {
        sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
    }

    // 重置绑定索引
    m_bindIndex = 1;

    int result = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &m_stmt, nullptr);
    if (result != SQLITE_OK) {
        setError(std::string("预编译语句失败: ") + sqlite3_errmsg(m_db));
        return false;
    }

    return true;
}

/**
 * @brief 绑定参数到预编译语句
 * @param index 参数索引（从1开始）
 * @param value 参数值
 * @return 绑定是否成功
 */
bool SqlQuery::bindValue(int index, const SqlValue &value) {
    clearError();

    if (!m_stmt) {
        setError("预编译语句为空");
        return false;
    }

    int result = SQLITE_OK;

    std::visit(
        [&](auto &&v) {
            using T = std::decay_t<decltype(v)>;

#ifdef QT_CORE_LIB
            if constexpr (std::is_same_v<T, QString>) {
                std::string str = v.toStdString();
                result = sqlite3_bind_text(m_stmt, index, str.c_str(), str.length(), SQLITE_TRANSIENT);
            } else if constexpr (std::is_same_v<T, QUuid>) {
                std::string str = v.toString(QUuid::WithoutBraces).toStdString();
                result = sqlite3_bind_text(m_stmt, index, str.c_str(), str.length(), SQLITE_TRANSIENT);
            } else if constexpr (std::is_same_v<T, QDateTime>) { // yyyy-MM-dd hh:mm:ss.zzz(UTC+8)
                std::string str = v.toString(Qt::ISODateWithMs).toStdString();
                result = sqlite3_bind_text(m_stmt, index, str.c_str(), str.length(), SQLITE_TRANSIENT);
            } else
#endif
#ifdef MY_DATETIME_ENABLED
                if constexpr (std::is_same_v<T, my::DateTime>) {
                std::string str = v.toISOString();
                result = sqlite3_bind_text(m_stmt, index, str.c_str(), str.length(), SQLITE_TRANSIENT);
            } else if constexpr (std::is_same_v<T, my::Date>) {
                std::string str = v.toISOString();
                result = sqlite3_bind_text(m_stmt, index, str.c_str(), str.length(), SQLITE_TRANSIENT);
            } else if constexpr (std::is_same_v<T, my::Time>) {
                std::string str = v.toISOString();
                result = sqlite3_bind_text(m_stmt, index, str.c_str(), str.length(), SQLITE_TRANSIENT);
            } else
#endif
#ifdef STDUUID_ENABLED
                if constexpr (std::is_same_v<T, uuids::uuid>) {
                std::string str = uuids::to_string(v);
                result = sqlite3_bind_text(m_stmt, index, str.c_str(), str.length(), SQLITE_TRANSIENT);
            } else
#endif
                if constexpr (std::is_same_v<T, int32_t>) {
                result = sqlite3_bind_int(m_stmt, index, v);
            } else if constexpr (std::is_same_v<T, int64_t>) {
                result = sqlite3_bind_int64(m_stmt, index, v);
            } else if constexpr (std::is_same_v<T, double>) {
                result = sqlite3_bind_double(m_stmt, index, v);
            } else if constexpr (std::is_same_v<T, bool>) {
                result = sqlite3_bind_int(m_stmt, index, v ? 1 : 0);
            } else if constexpr (std::is_same_v<T, std::string>) {
                result = sqlite3_bind_text(m_stmt, index, v.c_str(), v.length(), SQLITE_TRANSIENT);
            } else if constexpr (std::is_same_v<T, const char *>) {
                result = sqlite3_bind_text(m_stmt, index, v, -1, SQLITE_TRANSIENT);
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                if (v.empty())
                    result = sqlite3_bind_null(m_stmt, index);
                else
                    result = sqlite3_bind_blob(m_stmt, index, v.data(), v.size(), SQLITE_TRANSIENT);
            } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                result = sqlite3_bind_null(m_stmt, index);
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                result = sqlite3_bind_null(m_stmt, index);
            }
        },
        value);

    if (result != SQLITE_OK) {
        setError(std::string("绑定参数失败: ") + sqlite3_errmsg(m_db));
        return false;
    }

    return true;
}

/**
 * @brief 绑定多个参数到预编译语句
 * @param values 参数值向量
 * @return 绑定是否成功
 */
bool SqlQuery::bindValues(const std::vector<SqlValue> &values) {
    for (size_t i = 0; i < values.size(); ++i) {
        if (!bindValue(static_cast<int>(i + 1), values[i])) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 自动绑定参数到下一个位置
 * @param value 参数值
 * @return 绑定是否成功
 */
bool SqlQuery::addBindValue(const SqlValue &value) {
    if (!bindValue(m_bindIndex, value)) {
        return false;
    }
    m_bindIndex++;
    return true;
}

/**
 * @brief 自动绑定多个参数
 * @param values 参数值向量
 * @return 绑定是否成功
 */
bool SqlQuery::addBindValues(const std::vector<SqlValue> &values) {
    for (const auto &value : values) {
        if (!addBindValue(value)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 执行预编译语句
 * @return 执行是否成功
 */
bool SqlQuery::exec() {
    clearError();

    if (!m_stmt) {
        setError("预编译语句为空");
        return false;
    }

    int result = sqlite3_step(m_stmt);
    if (result != SQLITE_DONE && result != SQLITE_ROW) {
        setError(std::string("执行语句失败: ") + sqlite3_errmsg(m_db));
        return false;
    }

    return true;
}

/**
 * @brief 执行SQL语句（一次性）
 * @param sql SQL语句字符串
 * @return 执行是否成功
 */
bool SqlQuery::exec(const std::string &sql) {
    if (!prepare(sql)) {
        return false;
    }
    return exec();
}

/**
 * @brief 执行带参数的SQL语句
 * @param sqlTemplate SQL模板字符串（使用?作为占位符）
 * @param values 参数值向量
 * @return 执行是否成功
 */
bool SqlQuery::exec(const std::string &sqlTemplate, const std::vector<SqlValue> &values) {
    if (!prepare(sqlTemplate))
        return false;

    if (!bindValues(values))
        return false;

    return exec();
}

/**
 * @brief 移动到下一行结果
 * @return 是否有下一行数据
 */
bool SqlQuery::next() {
    clearError();

    if (!m_stmt) {
        setError("预编译语句为空");
        return false;
    }

    int result = sqlite3_step(m_stmt);
    if (result == SQLITE_ROW) {
        return true;
    } else if (result == SQLITE_DONE) {
        return false;
    } else {
        setError(std::string("执行语句失败: ") + sqlite3_errmsg(m_db));
        return false;
    }
}

/**
 * @brief 获取当前行的列值
 * @param index 列索引（从0开始）
 * @return 列值
 */
SqlValue SqlQuery::value(int index) const {
    if (!m_stmt) {
        return nullptr;
    }

    return valueFromStatement(m_stmt, index);
}

/**
 * @brief 获取当前行的列值
 * @param columnName 列名
 * @return 列值
 */
SqlValue SqlQuery::value(const std::string &columnName) const {
    if (!m_stmt) {
        return nullptr;
    }

    int columnCount = sqlite3_column_count(m_stmt);
    for (int i = 0; i < columnCount; ++i) {
        const char *name = sqlite3_column_name(m_stmt, i);
        if (name && columnName == name) {
            return value(i);
        }
    }

    return nullptr;
}

/**
 * @brief 获取当前行的所有数据
 * @return 当前行数据
 */
SqlRow SqlQuery::currentRow() const {
    SqlRow row;

    if (!m_stmt) {
        return row;
    }

    int columnCount = sqlite3_column_count(m_stmt);
    row.reserve(columnCount);

    for (int i = 0; i < columnCount; ++i) {
        const char *name = sqlite3_column_name(m_stmt, i);
        SqlValue val = value(i);
        row.emplace_back(name ? name : "", val);
    }

    return row;
}

/**
 * @brief 获取当前行的所有数据（映射）
 * @return 当前行数据（映射）
 */
SqlMap SqlQuery::currentMap() const {
    SqlMap row;

    if (!m_stmt) {
        return row;
    }

    int columnCount = sqlite3_column_count(m_stmt);
    for (int i = 0; i < columnCount; ++i) {
        const char *name = sqlite3_column_name(m_stmt, i);
        SqlValue val = value(i);
        row[name ? name : ""] = val;
    }

    return row;
}

/**
 * @brief 获取所有结果行
 * @return 完整结果集
 */
SqlResultSet SqlQuery::fetchAll() {
    SqlResultSet resultSet;

    while (next()) {
        resultSet.push_back(currentRow());
    }

    return resultSet;
}

/**
 * @brief 获取所有结果行（映射）
 * @return 完整结果集（映射）
 */
SqlMapResultSet SqlQuery::fetchAllMap() {
    SqlMapResultSet resultSet;

    while (next()) {
        resultSet.push_back(currentMap());
    }

    return resultSet;
}

/**
 * @brief 重置语句状态
 * @return 重置是否成功
 */
bool SqlQuery::reset() {
    clearError();

    if (!m_stmt) {
        setError("预编译语句为空");
        return false;
    }

    int result = sqlite3_reset(m_stmt);
    if (result != SQLITE_OK) {
        setError(std::string("重置语句失败: ") + sqlite3_errmsg(m_db));
        return false;
    }

    // 重置绑定索引
    m_bindIndex = 1;

    return true;
}

/**
 * @brief 清理绑定的参数
 * @return 清理是否成功
 */
bool SqlQuery::clearBindings() {
    clearError();

    if (!m_stmt) {
        setError("预编译语句为空");
        return false;
    }

    int result = sqlite3_clear_bindings(m_stmt);
    if (result != SQLITE_OK) {
        setError(std::string("清除绑定参数失败: ") + sqlite3_errmsg(m_db));
        return false;
    }

    // 重置绑定索引
    m_bindIndex = 1;

    return true;
}

/**
 * @brief 获取列数量
 * @return 列数量
 */
int SqlQuery::columnCount() const {
    if (!m_stmt) {
        return 0;
    }

    return sqlite3_column_count(m_stmt);
}

/**
 * @brief 获取列名
 * @param index 列索引（从0开始）
 * @return 列名
 */
std::string SqlQuery::columnName(int index) const {
    if (!m_stmt) {
        return "";
    }

    const char *name = sqlite3_column_name(m_stmt, index);
    return name ? name : "";
}

/**
 * @brief 获取受影响的行数
 * @return 受影响的行数
 */
int SqlQuery::rowsAffected() const {
    if (!m_db) {
        return 0;
    }

    return sqlite3_changes(m_db);
}

/**
 * @brief 获取最后插入的行ID
 * @return 最后插入的行ID
 */
int64_t SqlQuery::lastInsertRowId() const {
    if (!m_db) {
        return 0;
    }

    return sqlite3_last_insert_rowid(m_db);
}

/**
 * @brief 获取最后的错误信息
 * @return 错误信息
 */
std::string SqlQuery::lastError() const {
    return m_lastError;
}

#ifdef QT_CORE_LIB
/**
 * @brief 获取最后的错误信息（Qt QString）
 * @return 错误信息
 */
QString SqlQuery::lastErrorQt() const {
    return QString::fromStdString(m_lastError);
}
#endif

/**
 * @brief 检查是否有错误
 * @return 是否有错误
 */
bool SqlQuery::hasError() const {
    return m_hasError;
}

/**
 * @brief 获取参数数量
 * @return 参数数量
 */
int SqlQuery::parameterCount() const {
    if (!m_stmt)
        return 0;

    return sqlite3_bind_parameter_count(m_stmt);
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void SqlQuery::setError(const std::string &error) {
    m_lastError = error;
    m_hasError = true;
}

/**
 * @brief 清除错误状态
 */
void SqlQuery::clearError() {
    m_lastError.clear();
    m_hasError = false;
}

/**
 * @brief 从sqlite3_value获取SqlValue
 * @param value sqlite3值
 * @return SqlValue对象
 */
SqlValue SqlQuery::valueFromSqlite(sqlite3_value *value) {
    if (!value) {
        return nullptr;
    }

    int type = sqlite3_value_type(value);
    switch (type) {
    case SQLITE_INTEGER:
        return static_cast<int64_t>(sqlite3_value_int64(value));
    case SQLITE_FLOAT:
        return sqlite3_value_double(value);
    case SQLITE_TEXT: {
        const char *text = reinterpret_cast<const char *>(sqlite3_value_text(value));
        return text ? std::string(text) : std::string();
    }
    case SQLITE_BLOB: {
        // 对于BLOB数据，我们将其转换为std::vector<uint8_t>
        const void *blob = sqlite3_value_blob(value);
        int size = sqlite3_value_bytes(value);
        if (blob && size > 0) {
            const uint8_t *data = static_cast<const uint8_t *>(blob);
            return std::vector<uint8_t>(data, data + size);
        }
        return std::vector<uint8_t>();
    }
    case SQLITE_NULL:
    default:
        return nullptr;
    }
}

/**
 * @brief 从当前语句结果获取SqlValue
 * @param stmt 语句句柄
 * @param index 列索引
 * @return SqlValue对象
 */
SqlValue SqlQuery::valueFromStatement(sqlite3_stmt *stmt, int index) {
    if (!stmt) {
        return nullptr;
    }

    int type = sqlite3_column_type(stmt, index);
    switch (type) {
    case SQLITE_INTEGER:
        return static_cast<int64_t>(sqlite3_column_int64(stmt, index));
    case SQLITE_FLOAT:
        return sqlite3_column_double(stmt, index);
    case SQLITE_TEXT: {
        const char *text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, index));
        return text ? std::string(text) : std::string();
    }
    case SQLITE_BLOB: {
        // 对于BLOB(二进制大对象)数据，我们将其转换为std::vector<uint8_t>
        const void *blob = sqlite3_column_blob(stmt, index);
        int size = sqlite3_column_bytes(stmt, index);
        if (blob && size > 0) {
            const uint8_t *data = static_cast<const uint8_t *>(blob);
            return std::vector<uint8_t>(data, data + size);
        }
        return std::vector<uint8_t>();
    }
    case SQLITE_NULL:
    default:
        return nullptr;
    }
}