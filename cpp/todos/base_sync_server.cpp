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
#include "user_auth.h"
#include <QDateTime>
#include <QDebug>

BaseSyncServer::BaseSyncServer(QObject *parent)
    : QObject(parent),                                 // 父对象
      m_networkRequest(NetworkRequest::GetInstance()), // 网络请求对象
      m_setting(Setting::GetInstance()),               // 设置对象
      m_autoSyncTimer(new QTimer(this)),               // 自动同步定时器
      m_isSyncing(false),                              // 是否正在同步
      m_autoSyncInterval(30),                          // 自动同步间隔
      m_currentSyncDirection(Bidirectional)            // 当前同步方向
{
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
    m_autoSyncInterval = m_setting.get("sync/autoSyncInterval", 30).toInt();
    m_lastSyncTime = m_setting.get("sync/lastSyncTime", QString()).toString();

    // 如果启用了自动同步，启动定时器
    if (isAutoSyncEnabled()) {
        startAutoSyncTimer();
    }
}

BaseSyncServer::~BaseSyncServer() {
}

bool BaseSyncServer::isAutoSyncEnabled() const {
    return GlobalState::GetInstance().isAutoSyncEnabled();
}

void BaseSyncServer::setAutoSyncEnabled(bool enabled) {
    if (isAutoSyncEnabled() != enabled) {
        GlobalState::GetInstance().setIsAutoSyncEnabled(enabled);

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
        if (GlobalState::GetInstance().isAutoSyncEnabled()) {
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
    if (m_isSyncing) {
        qDebug() << "同步检查失败：正在进行同步操作，当前同步状态:" << m_isSyncing;
        qDebug() << "提示：如果同步状态异常，请调用resetSyncState()方法重置";
        return false;
    }

    if (m_serverBaseUrl.isEmpty()) {
        qDebug() << "同步检查失败：服务器基础URL为空";
        return false;
    }

    if (m_apiEndpoint.isEmpty()) {
        qDebug() << "同步检查失败：API端点为空";
        return false;
    }

    // 检查用户登录状态
    if (!UserAuth::GetInstance().isLoggedIn()) {
        qDebug() << "同步检查失败：用户未登录或令牌已过期";
        return false;
    }

    return true;
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