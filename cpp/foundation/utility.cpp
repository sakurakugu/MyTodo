/**
 * @file utility.cpp
 * @brief 工具类实现文件
 *
 * 该文件包含Utility类的实现，提供了一些通用的工具函数。
 *
 * @author Sakurakugu
 * @date 2025-09-23 18:32:17(UTC+8) 周二
 * @change 2025-09-24 00:55:58(UTC+8) 周三
 */
#include "foundation/utility.h"
#include <QCoreApplication>

Utility::Utility(QObject *parent) : QObject(parent) {}
Utility::~Utility() noexcept {}

/**
 * @brief 格式化日期时间
 *
 * 该函数根据输入的日期时间对象，返回一个格式化后的字符串表示。
 * 格式化规则如下：
 * - 若时间在当前时间的1分钟内，返回"刚刚"
 * - 若时间在当前时间的1小时内，返回"X分钟前"
 * - 若时间在当前时间的24小时内，返回"X:XX"
 * - 若时间在当前时间的24小时外，且在同一年内，返回"MM/DD"
 * - 若时间跨年度，返回"YYYY/MM/DD"
 *
 * @param dt 输入的日期时间对象
 * @return 格式化后的字符串表示
 */
QString Utility::formatDateTime(const QDateTime &dt) const {
    if (!dt.isValid()) {
        return "";
    }

    QDateTime now = QDateTime::currentDateTime();
    qint64 timeDiff = now.toMSecsSinceEpoch() - dt.toMSecsSinceEpoch();
    qint64 minutesDiff = timeDiff / (1000 * 60);
    qint64 hoursDiff = timeDiff / (1000 * 60 * 60);
    qint64 daysDiff = timeDiff / (1000 * 60 * 60 * 24);

    // 今天
    if (daysDiff == 0) {
        // 小于1分钟
        if (minutesDiff < 1) {
            return tr("刚刚");
        } else if (hoursDiff < 1) { // 小于1小时
            return tr("%1分钟前").arg(minutesDiff);
        } else { // 显示具体时间
            int hours = dt.time().hour();
            int minutes = dt.time().minute();
            return QString("%1:%2").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0'));
        }
    } else if (daysDiff == 1) { // 昨天
        return tr("昨天");
    } else if (daysDiff == 2) { // 前天
        return tr("前天");
    } else if (dt.date().year() == now.date().year()) { // 今年内
        int month = dt.date().month();
        int day = dt.date().day();
        return QString("%1/%2").arg(month, 2, 10, QChar('0')).arg(day, 2, 10, QChar('0'));
    } else { // 跨年
        int year = dt.date().year();
        int month = dt.date().month();
        int day = dt.date().day();
        return QString("%1/%2/%3")
            .arg(year, 4, 10, QChar('0'))
            .arg(month, 2, 10, QChar('0'))
            .arg(day, 2, 10, QChar('0'));
    }
}
