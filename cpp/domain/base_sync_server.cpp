/**
 * @file base_sync_server.cpp
 * @brief BaseSyncServer基类的实现文件
 *
 * 该文件实现了BaseSyncServer基类的通用同步功能。
 *
 * @author Sakurakugu
 * @date 2025-09-10 22:45:52(UTC+8) 周三
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */

#include "base_sync_server.h"
#include "config.h"
#include "global_state.h"
#include "user_auth.h"

#include <QDateTime>

BaseSyncServer::BaseSyncServer(UserAuth &userAuth, QObject *parent)
    : QObject(parent),                                 // 父对象
      m_networkRequest(NetworkRequest::GetInstance()), // 网络请求对象
      m_config(Config::GetInstance()),                 // 设置对象
      m_userAuth(userAuth),                            // 用户认证对象
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

    // 连接自动同步设置变化信号
    connect(&m_userAuth, &UserAuth::firstAuthCompleted, this, [this]() { 与服务器同步(); });

    // 从设置中加载自动同步配置
    m_autoSyncInterval = m_config.get("sync/autoSyncInterval", 30).toInt();
    m_lastSyncTime = QString("1970-01-01 00:00:00");

        开启自动同步计时器();
    
}

BaseSyncServer::~BaseSyncServer() {}

bool BaseSyncServer::isSyncing() const {
    return m_isSyncing;
}

void BaseSyncServer::setIsSyncing(bool syncing) {
    if (m_isSyncing != syncing) {
        m_isSyncing = syncing;
        emit syncingChanged();
    }
}

// 默认的同步操作实现
void BaseSyncServer::重置同步状态() {
    m_isSyncing = false;
    m_currentSyncDirection = Bidirectional;
    m_pushFirstInBidirectional = false;
    emit syncingChanged();
}

void BaseSyncServer::取消同步() {
    if (m_isSyncing) {
        setIsSyncing(false);
        emit syncCompleted(UnknownError, "同步已取消");
    }
    m_pushFirstInBidirectional = false;
}

// 网络请求处理（默认实现）
void BaseSyncServer::onNetworkRequestCompleted([[maybe_unused]] Network::RequestType type,
                                               [[maybe_unused]] const QJsonObject &response) {
    // 子类应该重写此方法来处理具体的响应
}

void BaseSyncServer::onNetworkRequestFailed([[maybe_unused]] Network::RequestType type,
                                            NetworkRequest::NetworkError error, const QString &message) {

    if (m_isSyncing) {
        setIsSyncing(false);

        SyncResult result = NetworkError;
        if (error == NetworkRequest::AuthenticationError) {
            result = AuthError;
        }

        emit syncCompleted(result, message);
    }
}

void BaseSyncServer::onAutoSyncTimer() {
    if (是否可以执行同步()) {
        与服务器同步();
    }
}

void BaseSyncServer::更新最后同步时间() {
    m_lastSyncTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

// TODO: 和检查同步前置条件重复了
bool BaseSyncServer::是否可以执行同步() const {
    // 检查是否可以执行同步
    if (m_isSyncing) {
        qDebug() << "同步检查失败：正在进行同步操作，当前同步状态:" << m_isSyncing;
        qDebug() << "提示：如果同步状态异常，请调用resetSyncState()方法重置";
        return false;
    }

    if (m_networkRequest.getServerBaseUrl().isEmpty()) {
        qDebug() << "同步检查失败：服务器基础URL为空";
        return false;
    }

    if (m_apiEndpoint.isEmpty()) {
        qDebug() << "同步检查失败：API端点为空";
        return false;
    }

    // 检查用户登录状态
    if (!m_userAuth.是否已登录()) {
        qDebug() << "同步检查失败：用户未登录或令牌已过期";
        return false;
    }

    return true;
}

void BaseSyncServer::开启自动同步计时器() {
    if (m_autoSyncTimer && m_autoSyncInterval > 0) {
        m_autoSyncTimer->start(m_autoSyncInterval * 60 * 1000); // 转换为毫秒
    }
}

void BaseSyncServer::停止自动同步计时器() {
    if (m_autoSyncTimer) {
        m_autoSyncTimer->stop();
    }
}

/**
 * @brief 检查同步前置条件
 * @param allowOngoingPhase 当为 true 时，如果当前处于同一次双向同步的后续阶段（例如先拉取再推送），
 *        即使 m_isSyncing == true 也不视为冲突；仅进行必要的配置与认证校验。
 *        默认 false，保持原有严格语义用于外部入口调用。
 */
void BaseSyncServer::检查同步前置条件(bool allowOngoingPhase) {
    // 1. 同步状态：仅在不允许阶段继续时严格限制
    if (m_isSyncing && !allowOngoingPhase) {
        qDebug() << "同步检查失败：正在进行同步操作，当前同步状态:" << m_isSyncing;
        qDebug() << "提示：如果同步状态异常，请调用resetSyncState()方法重置";
        emit syncCompleted(UnknownError, "无法同步：已有同步操作进行中");
        return;
    }

    // 2. 服务器基础URL
    if (m_networkRequest.getServerBaseUrl().isEmpty()) {
        qDebug() << "同步检查失败：服务器基础URL为空";
        emit syncCompleted(UnknownError, "无法同步：服务器基础URL未配置");
        return;
    }

    // 3. API端点
    if (m_apiEndpoint.isEmpty()) {
        qDebug() << "同步检查失败：API端点为空";
        emit syncCompleted(UnknownError, "无法同步：API端点未配置");
        return;
    }

    // 4. 用户登录 / token 状态
    if (!m_userAuth.是否已登录()) {
        qDebug() << "同步检查失败：用户未登录或令牌已过期";
        emit syncCompleted(AuthError, "无法同步：未登录");
        return;
    }
}