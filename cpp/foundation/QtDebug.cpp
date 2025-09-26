
#include "QtDebug.h"
#include <QDirIterator>

void 打印资源路径(const QString &path) {
    QDirIterator it(path, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
}