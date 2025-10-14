
/**
 * @file qt_debug.cpp
 * @brief Qt调试工具类的实现文件
 *
 * 该文件实现了QtDebug命名空间中的调试辅助功能。
 *
 * @author Sakurakugu
 * @date 2025-09-26 11:31:13(UTC+8) 周五
 * @change 2025-10-05 23:32:12(UTC+8) 周日
 */

#include "qt_debug.h"
#include "datetime.h"
#include "log_stream.h"
#include <QDirIterator>
#include <chrono>

// Windows 相关头文件
#ifdef Q_OS_WIN
#include <windows.h>
#endif

void QtDebug::打印资源路径(const QString &path) {
    QDirIterator it(path, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
}

void QtDebug::设置终端编码(int codePage) {
#ifdef Q_OS_WIN
    // Windows平台，设置控制台编码
    SetConsoleCP(codePage);
    SetConsoleOutputCP(codePage);
#endif
}

void QtDebug::测试用() {
    // 当前时间，qt格式
    QDateTime dt = QDateTime::currentDateTime();
    logWarning() << dt.toString(Qt::ISODateWithMs).toStdString();
    logWarning() << dt.toString(Qt::ISODate).toStdString();
    // 时间戳
    logWarning() << dt.toSecsSinceEpoch();
    logWarning() << dt.toMSecsSinceEpoch();

    logWarning() << my::DateTime::now(my::TimeZoneType::Local).toISOString();
    logWarning() << my::DateTime::utcNow();
}
