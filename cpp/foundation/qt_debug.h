/**
 * @file QtDebug.h
 * @brief Qt调试工具类的头文件
 *
 * 该文件定义了QtDebug命名空间，提供Qt应用程序的调试辅助功能。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六x
 * @change 2025-08-16 20:05:55(UTC+8) 周六x
 */
#pragma once

#include <QString>

namespace QtDebug {

void 打印资源路径(const QString &path = ":/");
void 设置终端编码();

} // namespace QtDebug