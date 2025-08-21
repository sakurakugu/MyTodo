#include "setting.h"

Setting::Setting(QObject *parent) : QObject(parent) {
}

Setting::~Setting() {
}

int Setting::getOsType() const {
#if defined(Q_OS_WIN)
    return 0;
#else
    return 1;
#endif
}
