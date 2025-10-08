/**
 * @file utility.h
 * @brief Utility类的头文件
 * 该文件定义了Utility类，提供通用的实用工具函数。
 * @author Sakurakugu
 * @date 2025-09-23 18:32:17(UTC+8) 周二
 * @change 2025-10-05 20:51:32(UTC+8) 周日
 */
#pragma once

#include <QDateTime>

// 定义DateType概念，限制模板参数为QDate或QDateTime
template <typename T>
concept DateType = std::same_as<T, QDate> || std::same_as<T, QDateTime>;

class Utility {
  public:
    Utility(const Utility &) = delete;
    Utility &operator=(const Utility &) = delete;
    Utility(Utility &&) = delete;
    Utility &operator=(Utility &&) = delete;

    static QString toRfc3339String(const QDateTime &dateTime);        // 将QDateTime转换为RFC 3339格式字符串
    static QJsonValue toRfc3339Json(const QDateTime &dateTime);       // 将QDateTime转换为RFC 3339格式的QJsonValue
    static QDateTime fromRfc3339String(const QString &rfc3339String); // 从RFC 3339格式字符串解析QDateTime
    static QString toIsoStringWithZ(const QDateTime &dateTime); // 将QDateTime转换为ISO 8601格式字符串（带毫秒和Z后缀）
    static QDateTime fromIsoString(const QString &isoString);   // 从ISO格式字符串解析QDateTime
    static QDateTime timestampToDateTime(const QVariant &timestampMs); // 将毫秒时间戳转换为QDateTime
    static QJsonValue timestampToIsoJson(const QVariant &timestampMs); // 将毫秒时间戳转换为ISO格式的QJsonValue
    static QDateTime fromJsonValue(const QJsonValue &jsonValue);       // 从QJsonValue解析时间戳并转换为QDateTime
    // TODO: 模板 static QDate fromJsonValue(const QJsonValue &jsonValue);       // 从QJsonValue解析日期并转换为QDate
    static QString currentUtcRfc3339();  // 获取当前UTC时间的RFC 3339格式字符串
    static QString currentUtcIsoWithZ(); // 获取当前UTC时间的ISO格式字符串（带Z后缀）

  private:
    Utility() = delete;
    ~Utility() noexcept = delete;

    // 内部辅助方法
    static QString ensureUtcSuffix(const QString &isoString);    // 确保ISO字符串包含Z后缀
    static QDateTime parseFlexibleIso(const QString &isoString); // 解析ISO格式字符串，支持带或不带毫秒
};
