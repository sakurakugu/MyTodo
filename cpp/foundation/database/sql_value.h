/**
 * @file sql_value.h
 * @brief SQL值类型定义
 * 支持多种数据类型的SQL值，用于参数绑定和结果获取
 * @note 若要新增类型，记得在下面的类型列表中也添加对应的处理，还有 sql_query.cpp 中的 bindValue 函数
 *
 * @author Sakurakugu
 * @date 2025-10-01 17:44 (UTC+8)
 */
#pragma once

#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#ifdef QT_CORE_LIB
#include <QDateTime>
#include <QString>
#include <QUuid>
#endif

#ifdef STDUUID_ENABLED
#include <uuid.h>
#endif

#ifdef MY_DATETIME_ENABLED
#include "datetime.h"
#endif

/**
 * @brief SQL值类型定义
 * 支持多种数据类型的SQL值，用于参数绑定和结果获取
 * @note 若要新增类型，记得在下面的类型列表中也添加对应的处理
 */
using SqlValue = std::variant< //
    int32_t,                   // 32位整数
    int64_t,                   // 64位整数
    double,                    // 双精度浮点数
    bool,                      // 布尔值
    std::string,               // 标准字符串
    const char *,              // C字符串
    std::vector<uint8_t>,      // 二进制数据(BLOB)
    std::nullptr_t,            // NULL值
    std::monostate             // 空值 / NULL
#ifdef QT_CORE_LIB
    ,
    QString,  // Qt字符串
    QUuid,    // Qt UUID
    QDateTime // Qt日期时间
#endif
#ifdef STDUUID_ENABLED
    ,
    uuids::uuid // UUID
#endif
#ifdef MY_DATETIME_ENABLED
    ,
    my::DateTime, // 日期时间
    my::Date,     // 日期
    my::Time      // 时间
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
 * @brief 判断 SqlValue 是否为空
 */
inline bool sqlValueIsNull(const SqlValue &value) {
    return std::holds_alternative<std::monostate>(value) || std::holds_alternative<std::nullptr_t>(value);
}

/**
 * @brief SQL值转换模板函数
 * 用于将SqlValue类型的值转换为指定类型T
 * @tparam T 目标类型，必须支持赋值操作
 * @param value 输入的SqlValue值
 * @return 转换后的T类型值
 * @throws std::runtime_error 当类型转换不兼容时抛出
 */
template <typename T> T sqlValueCast(const SqlValue &value) {
    return std::visit(
        [](auto &&v) -> T {
            using V = std::decay_t<decltype(v)>; // 提取实际类型

            // ---- 处理空值 ----
            if constexpr (std::is_same_v<V, std::monostate> || std::is_same_v<V, std::nullptr_t>) {
                if constexpr (std::is_arithmetic_v<T>) {
                    return static_cast<T>(0);
                } else if constexpr (std::is_same_v<T, bool>) {
                    return false;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    return {};
#ifdef QT_CORE_LIB
                } else if constexpr (std::is_same_v<T, QString>) {
                    return {};
#endif
                } else {
                    throw std::runtime_error("sqlValueCast: 空值 -> 不支持的类型");
                }

                // ---- 处理 const char* ----
            } else if constexpr (std::is_same_v<V, const char *>) {
                if constexpr (std::is_same_v<T, std::string>) {
                    return v ? std::string(v) : std::string();
#ifdef QT_CORE_LIB
                } else if constexpr (std::is_same_v<T, QString>) {
                    return v ? QString::fromUtf8(v) : QString();
#endif
                } else if constexpr (std::is_arithmetic_v<T>) {
                    return static_cast<T>(std::atof(v));
                } else {
                    throw std::runtime_error("sqlValueCast: const char* -> 不支持的类型");
                }

                // ---- BLOB ----
            } else if constexpr (std::is_same_v<V, std::vector<uint8_t>>) {
                if constexpr (std::is_same_v<T, std::string>) {
                    return std::string(v.begin(), v.end());
#ifdef QT_CORE_LIB
                } else if constexpr (std::is_same_v<T, QString>) {
                    return QString::fromUtf8(reinterpret_cast<const char *>(v.data()), static_cast<int>(v.size()));
#endif
                } else {
                    throw std::runtime_error("sqlValueCast: BLOB -> 不支持的类型");
                }

                // ---- 处理整数到布尔值的特殊转换 ----
            } else if constexpr (std::is_integral_v<V> && std::is_same_v<T, bool>) {
                return v != 0; // 非零值转换为true，零值转换为false

                // ---- 处理普通可转换类型 ----
            } else if constexpr (std::is_convertible_v<V, T>) { // 直接转换兼容类型
                return static_cast<T>(v);
            } else if constexpr (std::is_arithmetic_v<V> && std::is_arithmetic_v<T>) { // 算术类型转换
                return static_cast<T>(v);
#ifdef QT_CORE_LIB
                // ---- Qt 类型互转 ----
            } else if constexpr (std::is_same_v<V, QString> && std::is_same_v<T, std::string>) {
                return v.toStdString();
            } else if constexpr (std::is_same_v<V, std::string> && std::is_same_v<T, QString>) {
                return QString::fromStdString(v);
            } else if constexpr (std::is_same_v<V, QUuid> && std::is_same_v<T, std::string>) {
                return v.toString(QUuid::WithoutBraces).toStdString();
            } else if constexpr (std::is_same_v<V, std::string> && std::is_same_v<T, QUuid>) {
                return QUuid(QString::fromStdString(v));
            } else if constexpr (std::is_same_v<V, QDateTime> && std::is_same_v<T, std::string>) {
                return v.toString(Qt::ISODateWithMs).toStdString();
            } else if constexpr (std::is_same_v<V, std::string> && std::is_same_v<T, QDateTime>) {
                return QDateTime::fromString(QString::fromStdString(v), Qt::ISODateWithMs);
#endif
#ifdef STDUUID_ENABLED
                // ---- STDUUID 类型互转 ----
            } else if constexpr (std::is_same_v<V, uuids::uuid> && std::is_same_v<T, std::string>) {
                return uuids::to_string(v);
            } else if constexpr (std::is_same_v<V, std::string> && std::is_same_v<T, uuids::uuid>) {
                auto uuid_opt = uuids::uuid::from_string(v);
                return uuid_opt.value_or(uuids::uuid{});
#endif
#ifdef MY_DATETIME_ENABLED
                // ---- MY_DATETIME 类型互转 ----
            } else if constexpr (std::is_same_v<V, my::DateTime> && std::is_same_v<T, std::string>) {
                return v.toISOString();
            } else if constexpr (std::is_same_v<V, std::string> && std::is_same_v<T, my::DateTime>) {
                return my::DateTime::fromString(v);
            } else if constexpr (std::is_same_v<V, my::Date> && std::is_same_v<T, std::string>) {
                return v.toISOString();
            } else if constexpr (std::is_same_v<V, std::string> && std::is_same_v<T, my::Date>) {
                return my::Date::fromString(v);
            } else if constexpr (std::is_same_v<V, my::Time> && std::is_same_v<T, std::string>) {
                return v.toISOString();
            } else if constexpr (std::is_same_v<V, std::string> && std::is_same_v<T, my::Time>) {
                return my::Time::fromString(v);
#endif
            } else {
                throw std::runtime_error("sqlValueCast: 不兼容的类型转换");
            }
        },
        value);
}

/**
 * @brief 转换 SqlValue 为字符串表示（调试输出用）
 */
inline std::string sqlValueToString(const SqlValue &value) {
    return std::visit(
        [](auto &&v) -> std::string {
            using V = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<V, std::monostate> || std::is_same_v<V, std::nullptr_t>) {
                return "NULL";
            } else if constexpr (std::is_same_v<V, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<V, const char *>) {
                return v ? v : "";
            } else if constexpr (std::is_same_v<V, std::vector<uint8_t>>) {
                return std::string(v.begin(), v.end());
            } else if constexpr (std::is_arithmetic_v<V>) {
                return std::to_string(v);
#ifdef QT_CORE_LIB
            } else if constexpr (std::is_same_v<V, QString>) {
                return v.toStdString();
            } else if constexpr (std::is_same_v<V, QUuid>) {
                return v.toString(QUuid::WithoutBraces).toStdString();
            } else if constexpr (std::is_same_v<V, QDateTime>) {
                return v.toString(Qt::ISODateWithMs).toStdString();
#endif
#ifdef STDUUID_ENABLED
            } else if constexpr (std::is_same_v<V, uuids::uuid>) {
                return uuids::to_string(v);
#endif
#ifdef MY_DATETIME_ENABLED
            } else if constexpr (std::is_same_v<V, my::DateTime>) {
                return v.toISOString();
            } else if constexpr (std::is_same_v<V, my::Date>) {
                return v.toISOString();
            } else if constexpr (std::is_same_v<V, my::Time>) {
                return v.toISOString();
#endif
            } else {
                return "<不支持的类型>";
            }
        },
        value);
}

/**
 * @brief 从任意类型转换为 SqlValue
 * @tparam T 源类型
 * @param v 源值
 * @return SqlValue 转换后的SQL值
 * @note 若要新增类型，记得在bindValue中也添加对应的处理
 */
template <typename T> SqlValue sqlValueFrom(const T &v) {
    if constexpr (std::is_same_v<T, std::nullptr_t>) {
        return std::monostate{};
    } else if constexpr (std::is_same_v<T, SqlValue>) {
        return v;
    } else if constexpr (std::is_same_v<T, const char *>) {
        return v;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return v;
    } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
        return v;
    } else if constexpr (std::is_same_v<T, int32_t> || //
                         std::is_same_v<T, int64_t> || //
                         std::is_same_v<T, double> ||  //
                         std::is_same_v<T, bool>) {
        return v;
#ifdef QT_CORE_LIB
    } else if constexpr (std::is_same_v<T, QString>) {
        return v;
    } else if constexpr (std::is_same_v<T, QUuid>) {
        return v;
    } else if constexpr (std::is_same_v<T, QDateTime>) {
        return v;
#endif
#ifdef STDUUID_ENABLED
    } else if constexpr (std::is_same_v<T, uuids::uuid>) {
        return v;
#endif
#ifdef MY_DATETIME_ENABLED
    } else if constexpr (std::is_same_v<T, my::DateTime>) {
        return v;
    } else if constexpr (std::is_same_v<T, my::Date>) {
        return v;
    } else if constexpr (std::is_same_v<T, my::Time>) {
        return v;
#endif
    } else if constexpr (std::is_integral_v<T>) {
        // 任意整数类型，自动收窄
        if (v <= std::numeric_limits<int32_t>::max() && v >= std::numeric_limits<int32_t>::min()) {
            return static_cast<int32_t>(v);
        } else {
            return static_cast<int64_t>(v);
        }
    } else if constexpr (std::is_floating_point_v<T>) {
        return static_cast<double>(v);
    } else {
        static_assert(!sizeof(T *), "sqlValueFrom: 不支持的类型");
    }
}

/**
 * @brief 输出 SqlValue 到流（调试用）
 */
inline std::ostream &operator<<(std::ostream &os, const SqlValue &value) {
    os << sqlValueToString(value);
    return os;
}

/**
 * @brief 输出 SqlValue 到 QDebug 流（调试用）
 */
#ifdef QT_CORE_LIB
inline QDebug &operator<<(QDebug &dbg, const SqlValue &value) {
    dbg << sqlValueToString(value).c_str();
    return dbg;
}
#endif
