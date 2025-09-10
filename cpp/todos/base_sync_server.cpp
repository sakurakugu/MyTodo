/**
 * @file base_sync_server.cpp
 * @brief BaseSyncServer基类的实现文件
 *
 * 该文件实现了BaseSyncServer基类的通用同步功能。
 *
 * @author Sakurakugu
 * @date 2025-01-27 15:00:00(UTC+8) 周一
 * @version 0.4.0
 */

#include "base_sync_server.h"
#include "global_state.h"
#include <QDateTime>
#include <QDebug>

BaseSyncServer::BaseSyncServer(NetworkRequest *networkRequest, Setting *setting, QObject *parent)
    : QObject(parent), m_networkRequest(*networkRequest), m_setting(*setting), m_autoSyncTimer(new QTimer(this)),
      m_isAutoSyncEnabled(false), m_isSyncing(false), m_autoSyncInterval(30), m_currentSyncDirection(Bidirectional) {
    // 连接网络请求信号
    connect(&m_networkRequest, &NetworkRequest::requestCompleted, this, &BaseSyncServer::onNetworkRequestCompleted);
    connect(&m_networkRequest, &NetworkRequest::requestFailed, this, &BaseSyncServer::onNetworkRequestFailed);

    // 连接自动同步定时器
    connect(m_autoSyncTimer, &QTimer::timeout, this, &BaseSyncServer::onAutoSyncTimer);

    // 连接设置变化信号
    connect(&m_setting, &Setting::baseUrlChanged, this, &BaseSyncServer::onBaseUrlChanged);

    // 初始化服务器配置
    initializeServerConfig();

    // 从设置中加载自动同步配置
    m_isAutoSyncEnabled = m_setting.get("sync/autoSyncEnabled", false).toBool();
    m_autoSyncInterval = m_setting.get("sync/autoSyncInterval", 30).toInt();
    m_lastSyncTime = m_setting.get("sync/lastSyncTime", QString()).toString();

    // 如果启用了自动同步，启动定时器
    if (m_isAutoSyncEnabled) {
        startAutoSyncTimer();
    }
}

BaseSyncServer::~BaseSyncServer() {
    // 保存设置
    m_setting.save("sync/autoSyncEnabled", m_isAutoSyncEnabled);
    m_setting.save("sync/autoSyncInterval", m_autoSyncInterval);
    m_setting.save("sync/lastSyncTime", m_lastSyncTime);
}

// 属性访问器实现
bool BaseSyncServer::isAutoSyncEnabled() const {
    return m_isAutoSyncEnabled;
}

void BaseSyncServer::setAutoSyncEnabled(bool enabled) {
    if (m_isAutoSyncEnabled != enabled) {
        m_isAutoSyncEnabled = enabled;
        m_setting.save("sync/autoSyncEnabled", enabled);

        if (enabled) {
            startAutoSyncTimer();
        } else {
            stopAutoSyncTimer();
        }

        emit autoSyncEnabledChanged();
    }
}

bool BaseSyncServer::isSyncing() const {
    return m_isSyncing;
}

QString BaseSyncServer::lastSyncTime() const {
    return m_lastSyncTime;
}

int BaseSyncServer::autoSyncInterval() const {
    return m_autoSyncInterval;
}

void BaseSyncServer::setAutoSyncInterval(int minutes) {
    if (m_autoSyncInterval != minutes && minutes > 0) {
        m_autoSyncInterval = minutes;
        m_setting.save("sync/autoSyncInterval", minutes);

        // 如果自动同步已启用，重新启动定时器
        if (m_isAutoSyncEnabled) {
            startAutoSyncTimer();
        }

        emit autoSyncIntervalChanged();
    }
}

// 默认的同步操作实现
void BaseSyncServer::cancelSync() {
    if (m_isSyncing) {
        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(UnknownError, "同步已取消");
    }
}

void BaseSyncServer::resetSyncState() {
    m_isSyncing = false;
    m_currentSyncDirection = Bidirectional;
    emit syncingChanged();
}

// 配置管理实现
void BaseSyncServer::updateServerConfig(const QString &baseUrl, const QString &apiEndpoint) {
    m_serverBaseUrl = baseUrl;
    m_apiEndpoint = apiEndpoint;
    emit serverConfigChanged();
}

QString BaseSyncServer::getServerBaseUrl() const {
    return m_serverBaseUrl;
}

QString BaseSyncServer::getApiEndpoint() const {
    return m_apiEndpoint;
}

QString BaseSyncServer::getApiUrl(const QString &endpoint) const {
    QString url = m_serverBaseUrl;
    if (!url.endsWith('/')) {
        url += '/';
    }
    url += m_apiEndpoint;
    if (!endpoint.isEmpty()) {
        if (!url.endsWith('/')) {
            url += '/';
        }
        url += endpoint;
    }
    return url;
}

// 网络请求处理（默认实现）
void BaseSyncServer::onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response) {
    Q_UNUSED(type)
    Q_UNUSED(response)
    // 子类应该重写此方法来处理具体的响应
}

void BaseSyncServer::onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                            const QString &message) {
    Q_UNUSED(type)

    if (m_isSyncing) {
        m_isSyncing = false;
        emit syncingChanged();

        SyncResult result = NetworkError;
        if (error == NetworkRequest::AuthenticationError) {
            result = AuthError;
        }

        emit syncCompleted(result, message);
    }
}

void BaseSyncServer::onAutoSyncTimer() {
    if (canPerformSync()) {
        syncWithServer();
    }
}

void BaseSyncServer::onBaseUrlChanged(const QString &newBaseUrl) {
    m_serverBaseUrl = newBaseUrl;
    emit serverConfigChanged();
}

// 辅助方法实现
void BaseSyncServer::initializeServerConfig() {
    m_serverBaseUrl = m_setting.get("server/baseUrl", QString()).toString();
    // m_apiEndpoint 由子类设置
}

void BaseSyncServer::updateLastSyncTime() {
    m_lastSyncTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_setting.save("sync/lastSyncTime", m_lastSyncTime);
    emit lastSyncTimeChanged();
}

bool BaseSyncServer::canPerformSync() const {
    // 检查是否可以执行同步
    return !m_isSyncing && !m_serverBaseUrl.isEmpty() && !m_apiEndpoint.isEmpty();
    // 注意：用户认证检查应该由调用方负责
}

void BaseSyncServer::startAutoSyncTimer() {
    if (m_autoSyncTimer && m_autoSyncInterval > 0) {
        m_autoSyncTimer->start(m_autoSyncInterval * 60 * 1000); // 转换为毫秒
    }
}

void BaseSyncServer::stopAutoSyncTimer() {
    if (m_autoSyncTimer) {
        m_autoSyncTimer->stop();
    }
}