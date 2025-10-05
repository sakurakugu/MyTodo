
/**
 * @file QtDebug.cpp
 * @brief Qt调试工具类的实现文件
 *
 * 该文件实现了QtDebug命名空间中的调试辅助功能。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六x
 * @change 2025-08-16 20:05:55(UTC+8) 周六x
 */

#include "QtDebug.h"
#include <QDirIterator>

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

void QtDebug::设置终端编码() {
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
    // Windows平台，设置控制台编码
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
}
