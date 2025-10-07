/**
 * @file qml_update_checker.cpp
 * @brief QML更新检查器的实现文件
 *
 * 该文件实现了QmlUpdateChecker类，用于在QML中调用更新检查功能。
 *
 * @author Sakurakugu
 * @date 2025-10-07 19:45:00(UTC+8) 周二
 * @change 2025-10-07 19:45:00(UTC+8) 周二
 */
#include "qml_update_checker.h"
#include "version_checker.h"

QmlUpdateChecker::QmlUpdateChecker(QObject *parent)
    : QObject(parent), //
      m_versionChecker(new VersionChecker(this)) {
    // 信号转发
    connect(m_versionChecker, &VersionChecker::latestVersionChanged, this, &QmlUpdateChecker::latestVersionChanged);
    connect(m_versionChecker, &VersionChecker::hasUpdateChanged, this, &QmlUpdateChecker::hasUpdateChanged);
    connect(m_versionChecker, &VersionChecker::isCheckingChanged, this, &QmlUpdateChecker::isCheckingChanged);
    connect(m_versionChecker, &VersionChecker::updateUrlChanged, this, &QmlUpdateChecker::updateUrlChanged);
    connect(m_versionChecker, &VersionChecker::releaseNotesChanged, this, &QmlUpdateChecker::releaseNotesChanged);
    connect(m_versionChecker, &VersionChecker::autoCheckEnabledChanged, this,
            &QmlUpdateChecker::autoCheckEnabledChanged);
    connect(m_versionChecker, &VersionChecker::updateCheckCompleted, this, &QmlUpdateChecker::updateCheckCompleted);
    connect(m_versionChecker, &VersionChecker::updateCheckFailed, this, &QmlUpdateChecker::updateCheckFailed);
}

QmlUpdateChecker::~QmlUpdateChecker() {}

QString QmlUpdateChecker::currentVersion() const {
    return m_versionChecker->当前版本号();
}

QString QmlUpdateChecker::latestVersion() const {
    return m_versionChecker->最新版本号();
}

bool QmlUpdateChecker::hasUpdate() const {
    return m_versionChecker->是否有更新();
}

bool QmlUpdateChecker::isChecking() const {
    return m_versionChecker->是否正在检查();
}

QString QmlUpdateChecker::updateUrl() const {
    return m_versionChecker->更新URL();
}

QString QmlUpdateChecker::releaseNotes() const {
    return m_versionChecker->发布说明();
}

bool QmlUpdateChecker::autoCheckEnabled() const {
    return m_versionChecker->是否启用自动检查();
}

void QmlUpdateChecker::setAutoCheckEnabled(bool enabled) {
    return m_versionChecker->设置启用自动检查(enabled);
}

void QmlUpdateChecker::checkForUpdates() {
    return m_versionChecker->检查最新版本();
}

void QmlUpdateChecker::openDownloadPage() {
    return m_versionChecker->打开下载页面();
}

void QmlUpdateChecker::setCheckInterval(int hours) {
    return m_versionChecker->设置检查间隔(hours);
}
