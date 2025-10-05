/**
 * @file utility.cpp
 * @brief Utility工具类的实现文件
 *
 * 该文件包含Utility类的实现，提供了一些通用的工具函数。
 *
 * @author Sakurakugu
 * @date 2025-09-23 18:32:17(UTC+8) 周二
 * @change 2025-10-05 20:51:32(UTC+8) 周日
 */
#include "utility.h"

#include <QRegularExpression>
#include <QTimeZone>
#include <QJsonValue>

/**
 * @brief 将QDateTime转换为RFC 3339格式字符串
 * @param dateTime 要转换的日期时间对象
 * @return RFC 3339格式的字符串，如"2025-01-27T12:34:56.789Z"；无效时间返回空字符串
 * @note 自动转换为UTC时区，格式为yyyy-MM-ddTHH:mm:ss.zzzZ
 */
QString Utility::toRfc3339String(const QDateTime &dateTime) {
    if (!dateTime.isValid()) {
        return QString();
    }

    // 转换为UTC并格式化为RFC 3339格式
    return dateTime.toUTC().toString("yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");
}

/**
 * @brief 将QDateTime转换为RFC 3339格式的QJsonValue
 * @param dateTime 要转换的日期时间对象
 * @return RFC 3339格式的QJsonValue；无效时间返回QJsonValue()
 * @note 用于JSON序列化，自动处理null值
 */
QJsonValue Utility::toRfc3339Json(const QDateTime &dateTime) {
    if (!dateTime.isValid()) {
        return QJsonValue();
    }

    return QJsonValue(toRfc3339String(dateTime));
}

/**
 * @brief 从RFC 3339格式字符串解析QDateTime
 * @param rfc3339String RFC 3339格式的字符串
 * @return 解析后的QDateTime对象；解析失败返回无效的QDateTime
 * @note 支持带或不带毫秒的格式，自动处理时区信息
 */
QDateTime Utility::fromRfc3339String(const QString &rfc3339String) {
    if (rfc3339String.isEmpty()) {
        return QDateTime();
    }

    // 尝试解析RFC 3339格式
    // 支持格式：2025-01-27T12:34:56.789Z 或 2025-01-27T12:34:56Z
    QDateTime dt = QDateTime::fromString(rfc3339String, "yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");
    if (dt.isValid()) {
        dt.setTimeZone(QTimeZone::UTC);
        return dt;
    }

    // 尝试不带毫秒的格式
    dt = QDateTime::fromString(rfc3339String, "yyyy-MM-dd'T'HH:mm:ss'Z'");
    if (dt.isValid()) {
        dt.setTimeZone(QTimeZone::UTC);
        return dt;
    }

    // 尝试Qt的ISO格式解析作为后备
    return parseFlexibleIso(rfc3339String);
}

/**
 * @brief 将QDateTime转换为ISO 8601格式字符串（带毫秒和Z后缀）
 * @param dateTime 要转换的日期时间对象
 * @return ISO 8601格式的字符串，如"2025-01-27T12:34:56.789Z"；无效时间返回空字符串
 * @note 确保以Z结尾表示UTC时区，用于服务器通信
 */
QString Utility::toIsoStringWithZ(const QDateTime &dateTime) {
    if (!dateTime.isValid()) {
        return QString();
    }

    // 转换为UTC并使用Qt的ISO格式，确保以Z结尾
    QString isoString = dateTime.toUTC().toString(Qt::ISODateWithMs);
    return ensureUtcSuffix(isoString);
}

/**
 * @brief 从ISO格式字符串解析QDateTime
 * @param isoString ISO格式的字符串（支持Qt::ISODate格式）
 * @return 解析后的QDateTime对象；解析失败返回无效的QDateTime
 * @note 兼容Qt::ISODate格式，用于数据导入
 */
QDateTime Utility::fromIsoString(const QString &isoString) {
    if (isoString.isEmpty()) {
        return QDateTime();
    }

    return parseFlexibleIso(isoString);
}

/**
 * @brief 将毫秒时间戳转换为ISO格式的QJsonValue
 * @param timestampMs 毫秒时间戳（QVariant类型，支持数据库读取）
 * @return ISO格式的QJsonValue；无效时间戳返回QJsonValue()
 * @note 用于数据库数据导出到JSON
 */
QJsonValue Utility::timestampToIsoJson(const QVariant &timestampMs) {
    if (timestampMs.isNull()) {
        return QJsonValue();
    }

    bool ok = false;
    qint64 ms = timestampMs.toLongLong(&ok);
    if (!ok) {
        return QJsonValue();
    }

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(ms, QTimeZone::UTC);
    if (!dt.isValid()) {
        return QJsonValue();
    }

    return QJsonValue(dt.toString(Qt::ISODateWithMs));
}

/**
 * @brief 从QJsonValue解析时间戳并转换为QDateTime
 * @param jsonValue 包含时间字符串的QJsonValue
 * @return 解析后的QDateTime对象；解析失败返回无效的QDateTime
 * @note 自动检测ISO格式，用于JSON数据导入
 */
QDateTime Utility::fromJsonValue(const QJsonValue &jsonValue) {
    if (jsonValue.isNull() || !jsonValue.isString()) {
        return QDateTime();
    }

    return fromIsoString(jsonValue.toString());
}

/**
 * @brief 获取当前UTC时间的RFC 3339格式字符串
 * @return 当前UTC时间的RFC 3339格式字符串
 * @note 用于创建时间戳和同步标记
 */
QString Utility::currentUtcRfc3339() {
    return toRfc3339String(QDateTime::currentDateTimeUtc());
}

/**
 * @brief 获取当前UTC时间的ISO格式字符串（带Z后缀）
 * @return 当前UTC时间的ISO格式字符串
 * @note 用于日志记录和数据标记
 */
QString Utility::currentUtcIsoWithZ() {
    return toIsoStringWithZ(QDateTime::currentDateTimeUtc());
}

// 私有辅助方法实现

/**
 * @brief 确保ISO字符串以Z结尾（表示UTC时区）
 * @param isoString 输入的ISO格式字符串
 * @return 确保以Z结尾的字符串
 * @note 内部使用，用于RFC 3339和ISO 8601格式转换
 */
QString Utility::ensureUtcSuffix(const QString &isoString) {
    if (isoString.isEmpty()) {
        return isoString;
    }

    // 如果已经以Z结尾，直接返回
    if (isoString.endsWith('Z')) {
        return isoString;
    }

    // 如果以+00:00结尾，替换为Z
    if (isoString.endsWith("+00:00")) {
        return isoString.left(isoString.length() - 6) + 'Z';
    }

    // 否则直接添加Z
    return isoString + 'Z';
}

/**
 * @brief 从ISO格式字符串解析QDateTime（支持多种变体）
 * @param isoString ISO格式的字符串（支持Qt::ISODate格式）
 * @return 解析后的QDateTime对象；解析失败返回无效的QDateTime
 * @note 兼容Qt::ISODate格式，用于数据导入
 */
QDateTime Utility::parseFlexibleIso(const QString &isoString) {
    if (isoString.isEmpty()) {
        return QDateTime();
    }

    // 尝试Qt的标准ISO格式解析
    QDateTime dt = QDateTime::fromString(isoString, Qt::ISODate);
    if (dt.isValid()) {
        // 确保时区设置正确
        if (!dt.timeZone().isValid()) {
            dt.setTimeZone(QTimeZone::UTC);
        }
        return dt;
    }

    // 尝试带毫秒的ISO格式
    dt = QDateTime::fromString(isoString, Qt::ISODateWithMs);
    if (dt.isValid()) {
        if (!dt.timeZone().isValid()) {
            dt.setTimeZone(QTimeZone::UTC);
        }
        return dt;
    }

    // 尝试手动解析一些常见的变体格式
    QString cleanString = isoString.trimmed();

    // 处理以Z结尾但Qt无法解析的情况
    if (cleanString.endsWith('Z')) {
        // 移除Z并尝试解析，然后设置为UTC
        QString withoutZ = cleanString.left(cleanString.length() - 1);
        dt = QDateTime::fromString(withoutZ, "yyyy-MM-dd'T'HH:mm:ss.zzz");
        if (!dt.isValid()) {
            dt = QDateTime::fromString(withoutZ, "yyyy-MM-dd'T'HH:mm:ss");
        }
        if (dt.isValid()) {
            dt.setTimeZone(QTimeZone::UTC);
            return dt;
        }
    }

    // 处理+00:00时区标记
    if (cleanString.contains("+00:00")) {
        QString withoutTz = cleanString.replace("+00:00", "");
        dt = QDateTime::fromString(withoutTz, "yyyy-MM-dd'T'HH:mm:ss.zzz");
        if (!dt.isValid()) {
            dt = QDateTime::fromString(withoutTz, "yyyy-MM-dd'T'HH:mm:ss");
        }
        if (dt.isValid()) {
            dt.setTimeZone(QTimeZone::UTC);
            return dt;
        }
    }

    // 如果所有尝试都失败，返回无效的QDateTime
    return QDateTime();
}
