/**
 * @file version_checker.h
 * @brief 版本更新检查器的头文件
 *
 * 该文件定义了VersionChecker类，用于检查GitHub上的最新版本并与当前版本进行比较。
 *
 * @author Sakurakugu
 * @date 2025-10-07 19:35:00(UTC+8) 周二
 * @change 2025-10-07 19:35:00(UTC+8) 周二
 */

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QVersionNumber>

/**
 * @class VersionChecker
 * @brief 版本更新检查器，提供GitHub版本检查功能
 *
 * VersionChecker类是应用程序版本管理的核心组件，提供了完整的版本检查功能：
 *
 * **核心功能：**
 * - GitHub Releases API调用
 * - 版本号比较和解析
 * - 自动检查和手动检查
 * - 版本更新通知
 *
 * **高级特性：**
 * - 支持预发布版本过滤
 * - 版本检查间隔配置
 * - 网络错误处理
 * - 版本信息缓存
 *
 * **使用场景：**
 * - 应用启动时检查更新
 * - 设置页面手动检查更新
 * - 定期自动检查更新
 *
 * @note 所有网络操作都是异步的，通过信号槽机制通知结果
 */
class VersionChecker : public QObject {
    Q_OBJECT

  public:
    explicit VersionChecker(QObject *parent = nullptr);
    ~VersionChecker();

    // 属性访问器
    QString 当前版本号() const { return m_currentVersion; }
    QString 最新版本号() const { return m_latestVersion; }
    bool 是否有更新() const { return m_hasUpdate; }
    bool 是否正在检查() const { return m_isChecking; }
    QString 更新URL() const { return m_updateUrl; }
    QString 发布说明() const { return m_releaseNotes; }
    bool 是否启用自动检查() const { return m_autoCheckEnabled; }
    void 设置启用自动检查(bool enabled);

    // QML可调用方法
    void 检查最新版本();
    void 打开下载页面();
    void 设置检查间隔(int hours);

  signals:
    void latestVersionChanged();
    void hasUpdateChanged();
    void isCheckingChanged();
    void updateUrlChanged();
    void releaseNotesChanged();
    void autoCheckEnabledChanged();
    void updateCheckCompleted(bool hasUpdate, const QString &version);
    void updateCheckFailed(const QString &error);

  private slots:
    void onUpdateCheckFinished();
    void onAutoCheckTimer();

  private:
    // 内部方法
    void 执行版本检查();
    void 解析GitHub响应(const QJsonObject &response);
    bool 比较版本号(const QString &current, const QString &latest);
    void 设置最新版本(const QString &version);
    void 设置是否有更新(bool hasUpdate);
    void 设置是否正在检查(bool checking);
    void 设置更新URL(const QString &url);
    void 设置发布说明(const QString &notes);
    void 启动自动检查定时器();
    void 停止自动检查定时器();

    // 成员变量
    QString m_currentVersion;
    QString m_latestVersion;
    bool m_hasUpdate;
    bool m_isChecking;
    QString m_updateUrl;
    QString m_releaseNotes;
    bool m_autoCheckEnabled;
    int m_checkIntervalHours;

    QTimer *m_autoCheckTimer;

    // GitHub配置
    static const QString GITHUB_API_URL;
    static const QString GITHUB_REPO_OWNER;
    static const QString GITHUB_REPO_NAME;
    static const int DEFAULT_CHECK_INTERVAL_HOURS = 24;
};