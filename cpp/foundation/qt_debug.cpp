
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
#ifdef _WIN32
#include <windows.h>
#endif

void QtDebug::打印资源路径(const QString &path) {
    QDirIterator it(path, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
}

void QtDebug::设置终端编码(int codePage) {
#ifdef _WIN32
    // Windows平台，设置控制台编码
    SetConsoleCP(codePage);
    SetConsoleOutputCP(codePage);
#endif
}

void QtDebug::测试用() {
}
