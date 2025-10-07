/**
 * @file qml_update_checker.h
 * @brief QML更新检查器的头文件
 *
 * 该文件定义了QmlUpdateChecker类，用于在QML中调用更新检查功能。
 *
 * @author Sakurakugu
 * @date 2025-10-07 19:45:00(UTC+8) 周二
 * @change 2025-10-07 19:45:00(UTC+8) 周二
 */
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QVersionNumber>

class VersionChecker;

class QmlUpdateChecker : public QObject {
    Q_OBJECT

    // QML属性
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY latestVersionChanged)
    Q_PROPERTY(bool hasUpdate READ hasUpdate NOTIFY hasUpdateChanged)
    Q_PROPERTY(bool isChecking READ isChecking NOTIFY isCheckingChanged)
    Q_PROPERTY(QString updateUrl READ updateUrl NOTIFY updateUrlChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY releaseNotesChanged)
    Q_PROPERTY(bool autoCheckEnabled READ autoCheckEnabled WRITE setAutoCheckEnabled NOTIFY autoCheckEnabledChanged)

  public:
    explicit QmlUpdateChecker(QObject *parent = nullptr);
    ~QmlUpdateChecker();

    // 属性访问器
    QString currentVersion() const;
    QString latestVersion() const;
    bool hasUpdate() const;
    bool isChecking() const;
    QString updateUrl() const;
    QString releaseNotes() const;
    bool autoCheckEnabled() const;
    void setAutoCheckEnabled(bool enabled);

    // QML可调用方法
    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void openDownloadPage();
    Q_INVOKABLE void setCheckInterval(int hours);

  signals:
    void latestVersionChanged();
    void hasUpdateChanged();
    void isCheckingChanged();
    void updateUrlChanged();
    void releaseNotesChanged();
    void autoCheckEnabledChanged();
    void updateCheckCompleted(bool hasUpdate, const QString &version);
    void updateCheckFailed(const QString &error);

  private:
    // 成员变量
    VersionChecker *m_versionChecker;
};