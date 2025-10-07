/**
 * @file holiday_item.h
 * @brief HolidayItem类的头文件
 *
 * 该文件定义了HolidayItem类，用于表示单个节假日数据项。
 *
 * @author Sakurakugu
 * @date 2025-10-07 23:33:00(UTC+8) 周二
 */

#pragma once

#include <QDate>
#include <QJsonObject>
#include <QString>

/**
 * @class HolidayItem
 * @brief 节假日数据项类
 *
 * HolidayItem类表示单个节假日的信息：
 *
 * **核心属性：**
 * - 日期：节假日的具体日期
 * - 名称：节假日的名称（如"元旦"、"春节"等）
 * - 是否放假：该日期是否为休息日
 *
 * **功能特性：**
 * - JSON序列化和反序列化
 * - 数据验证
 * - 比较操作
 *
 * **使用场景：**
 * - 存储从API获取的节假日数据
 * - 在日历组件中显示节假日信息
 * - 计算工作日和休息日
 */
class HolidayItem {
  public:
    /**
     * @brief 默认构造函数
     */
    HolidayItem() = default;

    HolidayItem(const QDate &date, const QString &name, bool isOffDay);
    explicit HolidayItem(const QJsonObject &json);

    // Getter方法
    QDate date() const { return m_date; }
    QString name() const { return m_name; }
    bool isOffDay() const { return m_isOffDay; } ///< 是否为休息日

    // Setter方法
    void setDate(const QDate &date) { m_date = date; }
    void setName(const QString &name) { m_name = name; }
    void setIsOffDay(bool isOffDay) { m_isOffDay = isOffDay; }

    QJsonObject toJson() const;             ///< 转换为JSON对象
    bool fromJson(const QJsonObject &json); ///< 从JSON对象加载数据
    bool isValid() const;                   ///< 检查数据是否有效

    // 比较操作符（按日期比较）
    bool operator<(const HolidayItem &other) const;
    bool operator==(const HolidayItem &other) const;
    bool operator!=(const HolidayItem &other) const;

  private:
    QDate m_date;    ///< 节假日日期
    QString m_name;  ///< 节假日名称
    bool m_isOffDay; ///< 是否为休息日
};