/**
 * @file utility.h
 * @brief Utility类的头文件
 * 该文件定义了Utility类，提供通用的实用工具函数。
 * @author Sakurakugu
 * @date 2025-09-23 18:32:17(UTC+8) 周二
 * @change 2025-09-24 00:55:58(UTC+8) 周三
 */
#pragma once

#include <QDateTime>
#include <QObject>
#include <QtQml/qqmlregistration.h>

// 定义DateType概念，限制模板参数为QDate或QDateTime
template <typename T>
concept DateType = std::same_as<T, QDate> || std::same_as<T, QDateTime>;

class Utility : public QObject {
    Q_OBJECT

  public:
    // 单例模式
    static Utility &GetInstance() {
        static Utility instance;
        return instance;
    }

    // 禁用拷贝构造和赋值操作
    Utility(const Utility &) = delete;
    Utility &operator=(const Utility &) = delete;
    Utility(Utility &&) = delete;
    Utility &operator=(Utility &&) = delete;

    Q_INVOKABLE QString formatDateTime(const QDateTime &dt) const;
    template <typename DateType> DateType nullTime() const noexcept;                 // 获取空时间
    template <typename DateType> bool isNullTime(const DateType &dt) const noexcept; // 检查时间是否为空
    template <typename DateType> void setNullTime(DateType &dt) const noexcept;      // 设置时间为空

  private:
    explicit Utility(QObject *parent = nullptr);
    ~Utility() noexcept override;
};

// 模板函数实现移到类外部
template <typename DateType> bool Utility::isNullTime(const DateType &dt) const noexcept {
    return dt == nullTime<DateType>();
} // 检查时间是否为空

template <typename DateType> void Utility::setNullTime(DateType &dt) const noexcept {
    dt = nullTime<DateType>();
} // 设置时间为空

// ================== 模板特化 ==================
template <> inline QDateTime Utility::nullTime<QDateTime>() const noexcept {
    return QDateTime::fromString("1970-01-01T00:00:00", Qt::ISODate);
}

template <> inline QDate Utility::nullTime<QDate>() const noexcept {
    return QDate::fromString("1970-01-01", Qt::ISODate);
}
